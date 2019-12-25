/*++
	drivers/regulator/upi6631.c

	Copyright (c) 2012 WonderMedia Technologies, Inc.

	This program is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software Foundation,
	either version 2 of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful, but WITHOUT
	ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
	PARTICULAR PURPOSE.  See the GNU General Public License for more details.
	You should have received a copy of the GNU General Public License along with
	this program.  If not, see <http://www.gnu.org/licenses/>.

	WonderMedia Technologies, Inc.
	10F, 529, Chung-Cheng Road, Hsin-Tien, Taipei 231, R.O.C.
--*/

#include <linux/module.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/slab.h>
#include <linux/earlysuspend.h>
#include <linux/regulator/machine.h>
#include <mach/hardware.h>
#include <linux/interrupt.h>
#include <asm/unaligned.h>
#include <linux/cpufreq.h>

#undef DEBUG
#ifdef  DEBUG
#define DPRINTK(fmt, args...) do { printk(KERN_DEBUG fmt , ##args); } while (0)
#else
#define DPRINTK(fmt, args...) do { } while (0)
#endif

#define UPI_BUCK4_MAX_STAGE 21
#define MAX1586_V6_MAX_VSEL 3

#define UPI_BUCK4_MIN_UV  1000000 
#define UPI_BUCK4_MAX_UV  1500000

#define BUCK3_REG_IDX 1
#define BUCK4_REG_IDX 2
#define BUCK5_REG_IDX 3

extern int wmt_dvfs_running;
extern int wmt_suspend_target(unsigned, unsigned, unsigned);
extern char use_dvfs; 

#ifdef CONFIG_MTD_WMT_SF
extern int wmt_getsyspara(char *varname, unsigned char *varval, int *varlen);
#endif
static unsigned int g_i2cbus_id = 2;
static unsigned int g_pmic_en = 0;

struct upi_data {
	struct i2c_client *client;

	unsigned int min_uV;
	unsigned int max_uV;
	unsigned int current_uV;
	unsigned char enabled;
	unsigned int pwm_mode;

	struct regulator_dev *rdev;
};
struct upi_data *g_upidata;
DECLARE_WAIT_QUEUE_HEAD(upi6631_wait);
static unsigned int ostimer_int_pending = 0;
static unsigned int delay_time = 400;/*us*/
static int early_suspend_stage = 0;

#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend upi_pmic_early_suspend;
#endif

/*
 * use ostimer 3
 * delay_time:us
 */
static int set_ostimer_irq(unsigned int delay_time)
{
	unsigned int val, sw_count;
	unsigned int trigger_time;
	unsigned int remain_time;
	int ret = 0;
	val = REG32_VAL(OSTC_ADDR);
	sw_count = 300000;
	if ((val&0x02) == 0) {
		val |= 0x02;
		REG32_VAL(OSTC_ADDR) = val;
	}
	while(1) {
		val = REG32_VAL(OSTA_ADDR);
		if ((val&0x20) != 0x20)
			break;
		if (--sw_count == 0) {
			ret = -1;
			printk(KERN_ERR "Read OST Count Request Failed\n");
			break;
		}
	}
	val = REG32_VAL(OSCR_ADDR);
	trigger_time = val;
	sw_count = 300000;
	while(1) {
		val = REG32_VAL(OSTA_ADDR);
		if ((val&0x08) != 0x08)
			break;
		if (--sw_count == 0) {
			ret = -1;
			printk(KERN_ERR "OST Match3 Request Failed\n");
			break;
		}
	}

	/*overrun issue*/
	if ((0xFFFFFFFF - trigger_time < delay_time * 3)) {
		remain_time = 0xFFFFFFFF - trigger_time;
		remain_time = delay_time * 3 - remain_time;
		REG32_VAL(OSM3_ADDR) = remain_time;
	} else
		REG32_VAL(OSM3_ADDR) = trigger_time + delay_time * 3;
	REG32_VAL(OSTS_ADDR) = 0x08;
	REG32_VAL(OSTI_ADDR) |= 0x08;
	return ret;
}

static void clear_ostimer_irq(void)
{
	REG32_VAL(OSTS_ADDR) = 0x08;
	REG32_VAL(OSTI_ADDR) &= ~0x08;
}

static irqreturn_t wmt_ostimer_isr(int irq, void *dev_id)
{
	clear_ostimer_irq();
	ostimer_int_pending = 1;
	wake_up(&upi6631_wait);
	return IRQ_HANDLED;
}

static int i2cWriteToUPI(struct i2c_client *client, unsigned char bufferIndex,
	unsigned char const dataBuffer[], unsigned short dataLength)
{
	unsigned char buffer4Write[256];
	struct i2c_msg msgs[1] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = dataLength + 1,
			.buf = buffer4Write
		}
	};
	buffer4Write[0] = bufferIndex;
	memcpy(&(buffer4Write[1]), dataBuffer, dataLength);

	return i2c_transfer(client->adapter, msgs, 1);
}

#if 0
static int i2cReadFromUPI(struct i2c_client *client, unsigned char bufferIndex,
		unsigned char dataBuffer[], unsigned short dataLength)
{
	int ret;
	struct i2c_msg msgs[2] = {
		{
			.addr = client->addr,
			.flags = I2C_M_NOSTART,
			.len = 1,
			.buf = &bufferIndex
		},
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = dataLength,
			.buf = dataBuffer
		}
	};

	memset(dataBuffer, 0x00, dataLength);
	ret = i2c_transfer(client->adapter, msgs, 2);
	return ret;
}

static int upi_read_ready(struct i2c_client *client)
{
	unsigned char buf[1];
	buf[0] = 0;
	i2cReadFromUPI(client, BUCK4_REG_IDX, buf, 1);
	if (buf[0] & 0x20)
		return 0;
	return -1;

}
#endif
int upi6631_read_i2c(struct i2c_client *client, 
                u8 reg, int *rt_value, int b_single)
{
	struct i2c_msg msg[2];
	unsigned char data[4];
	int err;

	if (!client || !client->adapter)
		return -ENODEV;

	if (!rt_value)
		return -EINVAL;

	data[0] = reg;

	msg[0].addr = client->addr;
	msg[0].flags = 0 | I2C_M_NOSTART;
	msg[0].len = 1;
	msg[0].buf = (unsigned char *)data;

	msg[1].addr = client->addr;
	msg[1].flags = (I2C_M_RD);
	msg[1].len = b_single ? 1 : 2;
	msg[1].buf = (unsigned char *)data;

	err = i2c_transfer(client->adapter, msg, sizeof(msg)/sizeof(struct i2c_msg));

	if (err < 0) return err;

	if (b_single)
		*rt_value = data[0];
	else 
		*rt_value = get_unaligned_le16(data);

	return 0;
}
static int upi6631_set_PWM_mode(struct i2c_client *i2c) 
{
	u32 tmp_buf=0;
	int ret=0;
	struct i2c_client *client = NULL;

	printk("%s\n", __func__);
	if (i2c)
		client = i2c;
	else
		client = g_upidata->client;

	ret = upi6631_read_i2c(client, BUCK3_REG_IDX, &tmp_buf, 1);
	if (ret < 0)
		return ret;
	DPRINTK("%d = 0x%02X\n", BUCK3_REG_IDX, tmp_buf);

	tmp_buf |= BIT6;
	ret = i2cWriteToUPI(client, BUCK3_REG_IDX, (unsigned char const *)&tmp_buf, 1);
	if (ret < 0)
		return ret;

	ret = upi6631_read_i2c(client, BUCK4_REG_IDX, &tmp_buf, 1);
	if (ret < 0)
		return ret;
	DPRINTK("%d = 0x%02X\n", BUCK4_REG_IDX, tmp_buf);

	tmp_buf |= BIT6;
	ret = i2cWriteToUPI(client, BUCK4_REG_IDX, (unsigned char const *)&tmp_buf, 1);
	if (ret < 0)
		return ret;
	g_upidata->pwm_mode = 1;

	ret = upi6631_read_i2c(client, BUCK5_REG_IDX, &tmp_buf, 1);
	if (ret < 0)
		return ret;
	DPRINTK("%d = 0x%02X\n", BUCK5_REG_IDX, tmp_buf);

	tmp_buf |= BIT6;
	ret = i2cWriteToUPI(client, BUCK5_REG_IDX, (unsigned char const *)&tmp_buf, 1);
	if (ret < 0)
		return ret;
	g_upidata->pwm_mode = 1;

	return 0;

}

static int upi6631_set_auto_mode(struct i2c_client *i2c)
{
	u32 tmp_buf=0;
	int ret=0;
	struct i2c_client *client = NULL;

	printk("%s\n", __func__);
	if (i2c)
		client = i2c;
	else
		client = g_upidata->client;

	ret = upi6631_read_i2c(client, BUCK3_REG_IDX, &tmp_buf, 1);
	if (ret < 0)
		return ret;
	DPRINTK("%d = 0x%02X\n", BUCK3_REG_IDX, tmp_buf);
	tmp_buf &= ~BIT6;
	ret = i2cWriteToUPI(client, BUCK3_REG_IDX, (unsigned char const *)&tmp_buf, 1);
	if (ret < 0)
		return ret;

	ret = upi6631_read_i2c(client, BUCK4_REG_IDX, &tmp_buf, 1);
	if (ret < 0)
		return ret;
	DPRINTK("%d = 0x%02X\n", BUCK4_REG_IDX, tmp_buf);
	tmp_buf &= ~BIT6;
	ret = i2cWriteToUPI(client, BUCK4_REG_IDX, (unsigned char const *)&tmp_buf, 1);
	if (ret < 0)
		return ret;

	g_upidata->pwm_mode = 0;

	ret = upi6631_read_i2c(client, BUCK5_REG_IDX, &tmp_buf, 1);
	if (ret < 0)
		return ret;
	DPRINTK("%d = 0x%02X\n", BUCK5_REG_IDX, tmp_buf);
	tmp_buf &= ~BIT6;
	ret = i2cWriteToUPI(client, BUCK5_REG_IDX, (unsigned char const *)&tmp_buf, 1);
	if (ret < 0)
		return ret;

	return 0;
}

static int upi_buck4_set_voltage(struct regulator_dev *rdev, int min_uV, int max_uV,
			  unsigned *selector)
{
	struct upi_data *upi = rdev_get_drvdata(rdev);
	struct i2c_client *client = upi->client;
	u8 buck4_prog;
	int ret = 0;
	int retry_count = 10;

	if (min_uV > upi->max_uV || max_uV < upi->min_uV)
		return -EINVAL;
	if (max_uV > upi->max_uV)
		return -EINVAL;

	if (min_uV < upi->min_uV)
		min_uV = upi->min_uV;

	if ((min_uV - upi->min_uV) % 25000) 
		*selector = ((min_uV - upi->min_uV) / 25000) + 1;
	else
		*selector = (min_uV - upi->min_uV) / 25000;

	buck4_prog = *selector;

	dev_dbg(&client->dev, "changing voltage buck4 to %duv\n",
		min_uV);

	if (early_suspend_stage == 1)
		return -EINVAL;

	if (upi->pwm_mode == 1)
		buck4_prog |= BIT6;

	while (retry_count > 0) {
		ret = i2cWriteToUPI(client, BUCK4_REG_IDX, &buck4_prog, 1);
		if (ret >= 0)
			break;
		--retry_count;
	}
	if (ret < 0)
		return ret;
	/*
	while (upi_read_ready(client) == 1)
		schedule();
	*/
	ostimer_int_pending = 0;
	set_ostimer_irq(delay_time);
	wait_event(upi6631_wait, ostimer_int_pending);
	upi->current_uV = min_uV;
	return ret;
	
}
static int upi_buck4_get_voltage(struct regulator_dev *rdev)
{
	struct upi_data *upi = rdev_get_drvdata(rdev);
	return upi->current_uV;
}

static int upi_buck4_is_enabled(struct regulator_dev *rdev)
{
	struct upi_data *upi = rdev_get_drvdata(rdev);
	if (upi->enabled == 1)
		return 0;
	else
		return -EDOM;
	return 0;
}


#if 0
static int upi_buck4_list(struct regulator_dev *rdev, unsigned selector)
{
	struct upi_data *upi = rdev_get_drvdata(rdev);

	if (selector > MAX1586_V3_MAX_VSEL)
		return -EINVAL;
	return max1586_v3_calc_voltage(max1586, selector);
}
#endif

static struct regulator_ops upi_buck4_ops = {
	.set_voltage = upi_buck4_set_voltage,
	.is_enabled	= upi_buck4_is_enabled,
	.get_voltage	= upi_buck4_get_voltage,
#if 0
	.list_voltage = upi_buck4_list_voltage,
#endif
};

static struct regulator_desc upi_reg[] = {
	{
		.name = "wmt_corepower",
		.id = 0,
		.ops = &upi_buck4_ops,
		.type = REGULATOR_VOLTAGE,
		.n_voltages = UPI_BUCK4_MAX_STAGE,
		.owner = THIS_MODULE,
	},
};

static struct regulator_consumer_supply upi_buck4_supply =
	REGULATOR_SUPPLY("wmt_corepower", NULL);

static struct regulator_init_data upi_buck4_power = {
	.constraints = {
		.min_uV                 = UPI_BUCK4_MIN_UV,
		.max_uV                 = UPI_BUCK4_MAX_UV,
		.apply_uV               = true,
		.valid_modes_mask       = REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask         = REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS
					| REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies      = &upi_buck4_supply,
};

#if 0
static int upi_dump_reg() 
{
	u32 tmp_buf=0;
	int ret=0;
	struct i2c_client *client = g_upidata->client;

	/*
	ret = i2cReadFromUPI(client, BUCK3_REG_IDX, (u8 *)&tmp_buf, 1);
	*/
	ret = upi6631_read_i2c(client, BUCK3_REG_IDX, &tmp_buf, 1);
	if (ret < 0) return ret;
	printk(KERN_INFO "%s: %d = 0x%02X\n", __func__, BUCK3_REG_IDX, tmp_buf);

	ret = upi6631_read_i2c(client, BUCK4_REG_IDX, &tmp_buf, 1);
	if (ret < 0) return ret;
	printk(KERN_INFO "%s: %d = 0x%02X\n", __func__, BUCK4_REG_IDX, tmp_buf);

	ret = upi6631_read_i2c(client, BUCK5_REG_IDX, &tmp_buf, 1);
	if (ret < 0) return ret;
	printk(KERN_INFO "%s: %d = 0x%02X\n", __func__, BUCK5_REG_IDX, tmp_buf);
}
#endif

static void upi6631_delay_init(void)
{
	int ret=0;
	struct i2c_client *client = g_upidata->client;

	ret = upi6631_set_PWM_mode(client);
	if (ret) {
		printk(KERN_ERR "Can't set 'force PWM mode' .\n");
	}

	/*
	ret = upi_dump_reg();
	if (ret) {
		printk(KERN_ERR "Dump upi6631 register\n");
		return ret;
	}
	*/

}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void upi_early_suspend(struct early_suspend *h)
{
	int ret=0;
	/*
	struct i2c_client *client = g_upidata->client;
	u8 buck_reg_val=0;
	*/
	wmt_dvfs_running = 0;
	if (use_dvfs)
	  wmt_suspend_target(0, CPUFREQ_RELATION_L, 1);
	g_upidata->enabled = 0;
	early_suspend_stage = 1;

	printk(KERN_ERR "%s: =============\n", __func__);

	ret = upi6631_set_auto_mode(NULL);
	if (ret) {
		printk(KERN_ERR "%s: Buck mode can't set to be auto mode.\n", __func__);
		return;
	}

	/*
	ret = upi_dump_reg();
	if (ret) {
		printk(KERN_ERR "%s: dump error.\n", __func__);
		return;
	}
	*/

}
static void upi_late_resume(struct early_suspend *h)
{
	int ret=0;
	printk(KERN_ERR "%s: =============\n", __func__);

	ret = upi6631_set_PWM_mode(NULL);
	if (ret) {
		printk(KERN_ERR "%s: Buck mode can't set to be force PWM mode.\n", __func__);
		return;
	}

	/*
	ret = upi_dump_reg();
	if (ret) {
		printk(KERN_ERR "%s: dump error.\n", __func__);
		return;
	}
	*/
	g_upidata->enabled = 1;

	early_suspend_stage = 0;
	wmt_dvfs_running = 1;
}
#endif

static int __devinit upi_pmic_probe(struct i2c_client *client,
					const struct i2c_device_id *i2c_id)
{
	struct regulator_dev *rdev;
	struct regulator_init_data	*initdata;
	struct upi_data *upi;
	int ret = 0;

	printk("%s\n", __func__);

	upi = kzalloc(sizeof(struct upi_data) +
			sizeof(struct regulator_dev *),
			GFP_KERNEL);
	if (!upi)
		goto out;

	upi->client = client;

	upi->min_uV = UPI_BUCK4_MIN_UV;
	upi->max_uV = UPI_BUCK4_MAX_UV;
	upi->enabled = 1;

	rdev = upi->rdev;
	initdata = &upi_buck4_power;
	rdev = regulator_register(upi_reg, &client->dev,
				     initdata,
				     upi);

	i2c_set_clientdata(client, upi);
#ifdef CONFIG_HAS_EARLYSUSPEND
	upi_pmic_early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	upi_pmic_early_suspend.suspend = upi_early_suspend;
	upi_pmic_early_suspend.resume = upi_late_resume;
	register_early_suspend(&upi_pmic_early_suspend);
#endif
	g_upidata = upi;

	request_irq(IRQ_OST3, wmt_ostimer_isr,
			IRQF_DISABLED,
			"upi_pmic", NULL); 

	upi6631_delay_init();

	dev_info(&client->dev, "UPI regulator driver loaded\n");
	return 0;

out:
	return ret;
}

static int __devexit upi_pmic_remove(struct i2c_client *client)
{
	struct upi_data *upi = i2c_get_clientdata(client);

	regulator_unregister(upi->rdev);
	kfree(upi);

	return 0;
}

static const struct i2c_device_id upi6631_id[] = {
	{ "upi6631", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, upi6631_id);

static struct i2c_driver upi6631_pmic_driver = {
	.probe = upi_pmic_probe,
	.remove = __devexit_p(upi_pmic_remove),
	.driver		= {
		.name	= "upi6631",
		.owner	= THIS_MODULE,
	},
	.id_table	= upi6631_id,
};

static struct i2c_board_info upi_i2c_board_info = {
	.type          = "upi6631",
	.flags         = 0x00,
	.addr          = 0x66,
	.platform_data = NULL,
	.archdata      = NULL,
	.irq           = -1,
};

#ifdef CONFIG_MTD_WMT_SF
#define par_len 80
static int parse_pmic_param(void)
{
	int retval;
	unsigned char buf[par_len];
	unsigned char tmp_buf[par_len];
	int i = 0;
	int j = 0;
	int varlen = par_len;
	char *varname = "wmt.pmic.param";
	unsigned int pmic_en = 0;
	retval = wmt_getsyspara(varname, buf, &varlen);
	if (retval == 0) {
		pmic_en = (buf[i] - '0' == 1)?1:0;
		if (pmic_en == 0) {/*disable*/
			g_pmic_en = 0;
			return -1;
		}
		i += 2;
		for (; i < par_len; ++i) {
			if (buf[i] == ':')
				break;
			tmp_buf[j] = buf[i];
			++j;
		}
		if (strncmp(tmp_buf,"upi6631",7)) {
			g_pmic_en = 0;
			return -1;
		}
		++i;

		sscanf((buf + i), "%d:%d", &g_i2cbus_id, &delay_time);
	} else {
		g_pmic_en = 0;
		return -1;
	}
	g_pmic_en = 1;
	return 0;
}
#endif

static int __init upi_pmic_init(void)
{
	struct i2c_board_info *upi6631_i2c_bi;
	struct i2c_adapter *adapter = NULL;
	struct i2c_client *client   = NULL;
	int ret = 0;

#ifdef CONFIG_MTD_WMT_SF
	parse_pmic_param();
#endif
	if (g_pmic_en == 0) {
		printk("No upi pmic\n");
		return -ENODEV;
	}
	upi6631_i2c_bi = &upi_i2c_board_info;
	adapter = i2c_get_adapter(g_i2cbus_id);/*in bus 2*/

	if (NULL == adapter) {
		printk("can not get i2c adapter, client address error\n");
		ret = -ENODEV;
		return -1;
	}

	client = i2c_new_device(adapter, upi6631_i2c_bi);
	if (client == NULL) {
		printk("allocate i2c client failed\n");
		ret = -ENOMEM;
		return -1;
	}
	i2c_put_adapter(adapter);


	return i2c_add_driver(&upi6631_pmic_driver);
}
module_init(upi_pmic_init);

static void __exit upi_pmic_exit(void)
{
	i2c_del_driver(&upi6631_pmic_driver);
}
module_exit(upi_pmic_exit);

MODULE_AUTHOR("WonderMedia Technologies, Inc.");
MODULE_DESCRIPTION("WMT GPIO PMIC Driver");
MODULE_LICENSE("GPL");
