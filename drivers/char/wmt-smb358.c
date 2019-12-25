/*++
	drivers/char/wmt-smb358.c - SMB358 Battery charging driver

	Copyright (c) 2012  WonderMedia Technologies, Inc.

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
/* 2012/07/20 Diable OTG function because SMB358 A1 bug */ 
/*  2012/09/20 add  SMB347 (i2c=0x6a) */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <mach/hardware.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <linux/workqueue.h>
#include <mach/wmt_mmap.h>
#include <linux/slab.h>


//<ASUS-Wade1_Chen20121004+>
#include <linux/spinlock.h>
#define DRV_NOT_READY           0
#define DRV_OK                  1
static u32 drv_status = DRV_NOT_READY; 
DEFINE_MUTEX(charger_mutex);

#define SMB_CHARGING_EN 1

#define SMB_RETRY_COUNT 5

typedef enum {
        NO_CABLE=0,
        USB_ADAPTER,    /* usb plugg-in adapter */
        USB_PC,         /* usb plugg-in PC */
} cable_type_t;
//<ASUS-Wade1_Chen20121004->

//<ASUS-Darren1_Chang20121130+>
extern void usb_to_battery_callback(u32 usb_cable_state);
#define SUSGPIO1_EVENT_ID 0x20
//<ASUS-Darren1_Chang20121130->
//
static int __devinit smb_i2c_probe(struct i2c_client *i2c,
			     const struct i2c_device_id *id);
#if 1
static unsigned int smb_read(unsigned int reg);
#endif

static int smb_write(unsigned int reg, unsigned int value);
static unsigned int g_battery_charging_en;
static unsigned int g_i2cbus_id = 3;

#ifdef CONFIG_MTD_WMT_SF
extern int wmt_getsyspara(char *varname, unsigned char *varval, int *varlen);
#endif

//#define SMB358 1


#define smb_I2C_ADDR    0x6a // SMB347 new board 20120912 
#define SMB_NAME "smb347"



#define SUSGPIO0_EVENT_ID 0x10
static struct i2c_client *smb_client;

static struct workqueue_struct *smb_wq;
struct work_struct smb_work;

extern void pmc_enable_wakeup_event(unsigned int wakeup_event, unsigned int type);
extern void pmc_disable_wakeup_event(unsigned int wakeup_event);
extern int pmc_register_callback(unsigned int wakeup_event, void (*callback)(void *), void *callback_data);
extern int pmc_unregister_callback(unsigned int wakeup_event);
extern unsigned int pmc_get_wakeup_status(void);

static int smb_write_retry(u8 reg, u8 val)
{
        int i=SMB_RETRY_COUNT;
        int ret=0;

        while(i--) {
                ret = smb_write(reg, val);
                if (!ret) break;
        }

        return ret;
}

static unsigned int smb_read_retry(u8 reg)
{
        int i=SMB_RETRY_COUNT;
        int val;

        while(i--) {
                val = smb_read(reg);
                if (val >= 0) break;
        }
        return val;
}

int init_hw_config(void)
{
        int ret;
        int step=0;

        printk(KERN_INFO "[%s]: enter\n", __func__);

        mutex_lock(&charger_mutex);

        // Step 1: turn to I2C control
        step = 10;
        ret = smb_write_retry(0x30, 0xC0);  // Allow volatile writing. 
        if (ret) goto err_config;

        step = 11;
        ret = smb_write_retry(0x09, 0x25); //OTG i2c control, active low, log battery 2.9V threshold
        if (ret) goto err_config;
        
        // Step 2: Disable OTG
        step = 20;
        ret = smb_write_retry(0x30, 0xC0);
        if (ret) goto err_config;
       
        // Step 3: Set to be 250mA
        step = 30;
        ret = smb_write_retry(0x0A, 0x34); //temp threshold 130oC, 250mA
        if (ret) goto err_config;

        mutex_unlock(&charger_mutex);

        return 0;

err_config:
        mutex_unlock(&charger_mutex);
        printk(KERN_ERR "[%s]: err %d, %d\n", __func__, ret, step);

        return ret;
}

int is_plug_device(void)
{
	u32 reg;

	reg = REG32_VAL(0xFE110000) & 0x00200000;  //  read sus_gpio0 status 

        return !reg ? 1 : 0;
}

//<ASUS-Darren1_Chang20121130+>
int is_plug_ac_device(void)
{
	u32 reg;

        pr_info("REG32_VAL(0xFE110000) = 0x%08X \n", REG32_VAL(0xFE110000));
        reg = (REG32_VAL(0xFE110000) & 0x00400000) && (REG32_VAL(0xFE110000) & 0x00200000);  //  read sus_gpio1 status

	return reg ? 1 : 0;
}
//<ASUS-Darren1_Chang20121130->

static void smb_enable_irq(void)
{
	pmc_enable_wakeup_event(SUSGPIO0_EVENT_ID, 4);
	pmc_enable_wakeup_event(SUSGPIO1_EVENT_ID, 4);	//<ASUS-Darren1_Chang20121130+>
}

static void smb_disable_irq(void)
{
	pmc_disable_wakeup_event(SUSGPIO0_EVENT_ID);
	pmc_disable_wakeup_event(SUSGPIO1_EVENT_ID);	//<ASUS-Darren1_Chang20121130+>
}

int smb_charging_op(int do_charging)
{
        int ret_val=0;
        int ret=-EINVAL;
        int step=0;

        pr_info("[%s] enter %d\n", __func__, do_charging);

        mutex_lock(&charger_mutex);

        if (is_plug_device()) {
                pr_err("[%s] not plug ac device\n", __func__);
                WARN_ON(1);
                goto err_out;
        }

        //turn to i2c control, enable charger.
        step = 10;
        ret = smb_read_retry(0x06);
        if (ret < 0) goto err_out;
        ret_val = ret;

        if (do_charging) {
                ret_val |= BIT5;
        }else {
                ret_val &= ~BIT5;
        }

        step = 11;
        ret = smb_write_retry(0x30, 0xC0);  // Allow volatile writing. 
        if (ret) goto err_out;

        step = 20;
        ret = smb_write_retry(0x06, ret_val);
        if (ret < 0) goto err_out;

        mutex_unlock(&charger_mutex);
        return 0;


err_out:
        mutex_unlock(&charger_mutex);

        pr_err("[%s] operation fail. (%d, %d)\n", __func__, step, ret);
        return ret;
}
EXPORT_SYMBOL(smb_charging_op);

static int smb_suspend(struct i2c_client *client, pm_message_t mesg)
{
        int ret=0;

	printk("[%s]\n", __func__);

	smb_disable_irq();
	cancel_work_sync(&smb_work);
        init_hw_config();

	return ret;
}

static void smb_shutdown(struct i2c_client *client)
{

        init_hw_config();
}

static int smb_resume(struct i2c_client *client)
{
	unsigned int wakeup_sts;

	printk("[%s] enter\n", __func__);

	wakeup_sts = pmc_get_wakeup_status();
	queue_work(smb_wq, &smb_work);
	smb_enable_irq();

	return 0;
}



static const struct i2c_device_id smb_i2c_id[] = {
	{ SMB_NAME, 1 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, smb_i2c_id);

static struct i2c_driver smb_i2c_driver = {
	.driver = {
		.name = SMB_NAME,
		.owner = THIS_MODULE,
	},
	.probe = smb_i2c_probe,
	.id_table = smb_i2c_id,
	.suspend = smb_suspend,
	.resume = smb_resume,
        .shutdown = smb_shutdown,
};

struct smb_conf_struct {
	unsigned char bus_id;	    // 0: bus-0  
	                               // 1: bus-1
	                               // 2: bus-2
	struct i2c_client *client;
	struct work_struct work;

	struct i2c_adapter *i2c_adap;								   
};
static struct smb_conf_struct smb_conf;

static struct i2c_board_info __initdata smb_board_info[] = {
	{
		I2C_BOARD_INFO(SMB_NAME, smb_I2C_ADDR),
	},
};

static void smb_irq_callback(void *data)
{
	queue_work(smb_wq, &smb_work);
}


#ifdef CONFIG_MTD_WMT_SF
static void parse_arg(void)
{
	int retval;
	unsigned char buf[80];
	unsigned char tmp_buf[80];
	int varlen = 80;
	char *varname = "wmt.bc.param";
	int i = 0;
	int j = 0;
	retval = wmt_getsyspara(varname, buf, &varlen);
	if (retval == 0) {
		for (i = 0; i < 80; ++i) {
			if (buf[i] == ':')
				break;
			g_battery_charging_en = (buf[i] - '0' == 1)?1:0;
			if (g_battery_charging_en == 0)/*disable*/
				return;
		}
		++i;
		for (; i < 80; ++i) {
			if (buf[i] == ':')
				break;
			tmp_buf[j] = buf[i];
			++j;
		}
		if (!strncmp(tmp_buf,SMB_NAME,6))
			g_battery_charging_en = 1;
		else
			g_battery_charging_en = 0;
		++i;
		g_i2cbus_id = buf[i] - '0';
	} else
		g_battery_charging_en = 0;
}
#endif
static int smb_modinit(void)
{
	int ret = 0;
	struct i2c_adapter *adapter = NULL;
	struct i2c_client *client   = NULL;

	printk(" smb_modinit \n");
#ifdef CONFIG_MTD_WMT_SF
	parse_arg();
#endif

	if (!g_battery_charging_en) {
		printk("NO SMB358/347 \n");
		return -EIO;
	}
	smb_conf.bus_id = g_i2cbus_id;
	adapter  = i2c_get_adapter(smb_conf.bus_id);
	smb_conf.i2c_adap = adapter;
	if (adapter == NULL) {
		printk("can not get smb i2c adapter, client address error");
		return -ENODEV;
	}
	if ((client=i2c_new_device(adapter, smb_board_info)) == NULL) {
		printk("allocate smb i2c client failed");
		return -ENOMEM;
	}
	
	ret = i2c_add_driver(&smb_i2c_driver);
	if(ret ) 
		printk("SMB 358/347 i2c fail\n");

	return ret;
}
module_init(smb_modinit);

int smb_OTG_pluggin_device(void)
{
	int ret=0;
        int step=0;

        printk(KERN_INFO "[%s] device pluggin \n", __func__);

        if (drv_status == DRV_NOT_READY)
                return -ENODEV;

        mutex_lock(&charger_mutex);
        //allow non-volatile register could be written and disable OTG.
        step = 10;
        ret = smb_write_retry(0x30, 0xC0);
        if (ret) goto err_config;

        //Set OTG current limit to 250mA, UVLO 2.7V
        step = 20;
        ret = smb_write_retry(0x0A, 0x34);
        if (ret) goto err_config;

        step = 30;
        ret = smb_write_retry(0x30, 0xD8); //enable OTG function, charging disable
        if (ret) goto err_config;

        step = 40;
        ret = smb_write_retry(0x0A, 0x3C); //limit to 750mA
        if (ret) goto err_config;

        step = 50;
        ret = smb_write_retry(0x09, 0x65); //RID enable, auto OTG
        if (ret) goto err_config;

        mutex_unlock(&charger_mutex);

        printk(KERN_INFO "[%s] device pluggin OK\n", __func__);

       return 0;

err_config:
       mutex_unlock(&charger_mutex);
       printk(KERN_ERR "[%s] err for config. %d, %d\n", __func__, ret, step);
       return ret;
}


static void smb_work_func(struct work_struct *work)
{
        if (is_plug_device()) {
                printk(KERN_INFO "[%s] enter \n", __func__);
                smb_OTG_pluggin_device();
        }

//<ASUS-Darren1_Chang20121130+>
        if (is_plug_ac_device()) {
        		printk(KERN_INFO "[%s] ac plug-in enter \n", __func__);
        		if (smb_read_retry(0x3E) & 0x04) {
            		printk(KERN_INFO "USB_PC \n");
					usb_to_battery_callback(USB_PC);
        		} else {	//smb_read_retry(0x3E) & 0x01
            		printk(KERN_INFO "USB_ADAPTER \n");
					usb_to_battery_callback(USB_ADAPTER);
        		}
        } else {
        	if (!is_plug_device()) {	//USB_ADAPTER or USB_PC
        		printk(KERN_INFO "[%s] ac plug-out enter \n", __func__);
        		if (!((smb_read_retry(0x3E) & 0x04) && (smb_read_retry(0x3E) & 0x01))) {
        			printk(KERN_INFO "NO_CABLE \n");
        			usb_to_battery_callback(NO_CABLE);
        		}
        	}
        }
//<ASUS-Darren1_Chang20121130+>
}


static int __devinit smb_i2c_probe(struct i2c_client *client,
			     const struct i2c_device_id *id)
{
	struct smb_conf_struct  *smb;
	int ret = 0;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk("SMB  I2C_FUNC_I2C\n");
	}

	smb = kzalloc(sizeof(*smb), GFP_KERNEL);
	if(smb == NULL)
		printk("SMB  FAIL\n");

	smb->client = client;
	smb_client = client;
	i2c_set_clientdata(client,smb);

	printk(" smb_i2c_probe \n");

	smb_wq = create_workqueue("smb_wq");
	if(smb_wq == NULL)
               printk("smb work fail \n");

	INIT_WORK(&smb_work,smb_work_func);

        /* initiallize */
        ret = init_hw_config();
        if (ret) {
                printk("[%s]: Fail to init smb, %d\n", __func__, ret); 
        }

	ret = pmc_register_callback(SUSGPIO0_EVENT_ID, smb_irq_callback, client);
	ret |= pmc_register_callback(SUSGPIO1_EVENT_ID, smb_irq_callback, client);	//<ASUS-Darren1_Chang20121130+>
	if (ret == 0)
		smb_enable_irq();

        drv_status = DRV_OK; //<ASUS-Wade1_Chen20121015+>

        /* detect if device already plug-in */
        if (is_plug_device()) {
                smb_OTG_pluggin_device();
        }

//<ASUS-Darren1_Chang20121130+>
        if (is_plug_ac_device()) {
        		if (smb_read_retry(0x3E) & 0x04) {
            		printk(KERN_INFO "USB_PC \n");
					usb_to_battery_callback(USB_PC);
        		} else {	//smb_read_retry(0x3E) & 0x01
            		printk(KERN_INFO "USB_ADAPTER \n");
					usb_to_battery_callback(USB_ADAPTER);
        		}
        } else {
        	if (!is_plug_device()) {	//USB_ADAPTER or USB_PC
        		if (!((smb_read_retry(0x3E) & 0x04) && (smb_read_retry(0x3E) & 0x01))) {
        			printk(KERN_INFO "NO_CABLE \n");
        			usb_to_battery_callback(NO_CABLE);
        		}
        	}
        }
//<ASUS-Darren1_Chang20121130+>

	return ret;
}

static int smb_write(unsigned int reg, unsigned int value)
{
	int count;
	struct i2c_msg msg[1];
	unsigned char buf[2];
	count=10;

	while (count--) {
		buf[0] = reg;
		buf[1] = value;
		msg[0].addr = smb_I2C_ADDR;
		msg[0].flags = 0 ;
		msg[0].flags &= ~(I2C_M_RD);
		msg[0].len = 2;
		msg[0].buf = buf;
	
		if (i2c_transfer(smb_conf.i2c_adap, msg, ARRAY_SIZE(msg)) == ARRAY_SIZE(msg))
			return 0;
	} 
	printk(" error: i2c write reg[%02x]=%02x ", reg, value);
	return -1;
}

#if 1
static unsigned int smb_read(unsigned int reg)
{
	unsigned char buf[1];
	struct i2c_msg msg[2];
	
	msg[0].addr = smb_I2C_ADDR;
	msg[0].flags = (2 | I2C_M_NOSTART);
	msg[0].len = 1;
	msg[0].buf = (unsigned char *)&reg;

	msg[1].addr = smb_I2C_ADDR;
	msg[1].flags = (I2C_M_RD | I2C_M_NOSTART);
	msg[1].len = 1;
	msg[1].buf = buf;

	if (i2c_transfer(smb_conf.i2c_adap, msg, ARRAY_SIZE(msg)) == ARRAY_SIZE(msg))
		return buf[0];

	printk(" error: i2c read reg[%02x]", reg);
	return -1;
	
}
#endif

static void __exit smb_exit(void)
{
	i2c_del_driver(&smb_i2c_driver);

}
module_exit(smb_exit);

MODULE_DESCRIPTION("WMT SMB358/SMB347 driver");
MODULE_AUTHOR("WonderMedia Technologies, Inc.");
MODULE_LICENSE("GPL");
