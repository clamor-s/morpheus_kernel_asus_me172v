/*
 * Copyright (c) 2012, ASUSTek, Inc. All Rights Reserved.
 * Written by wade chen Wade1_Chen@asus.com
 */
#include <linux/module.h>
#include <linux/param.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <asm/unaligned.h>
#include <linux/interrupt.h>
#include <linux/idr.h>
#include <mach/common_def.h>
#include <mach/hardware.h>
#include <mach/irqs.h>

#include "asus_battery.h"
#include "bq27520_battery_i2c.h"
#include "bq27520_proc_fs.h"

#define RETRY_COUNT 3

//extern function 
extern int wmt_getsyspara(char *varname, char *varval, int *varlen);
extern void pmc_enable_wakeup_event(unsigned int wakeup_event, unsigned int type);
extern void pmc_disable_wakeup_event(unsigned int wakeup_event);
extern int pmc_register_callback(unsigned int wakeup_event, void (*callback)(void *), void *callback_data);
extern int pmc_unregister_callback(unsigned int wakeup_event);
extern unsigned int pmc_get_wakeup_status(void);
//

struct battery_dev_info batt_dev_info;
DEFINE_MUTEX(batt_dev_info_mutex);

static struct battery_low_config batlow ;
static struct asus_batgpio_set batlow_gp ;
//static struct workqueue_struct *batlow_wq=NULL;
static struct work_struct batlow_work;
static struct dev_func bq27520_tbl;

static struct asus_batgpio_set  gpio_cell_selector;
/*
 * i2c specific code
 */
#if defined(CONFIG_ASUS_BAT_LOW_EVENT) && CONFIG_ASUS_BAT_LOW_EVENT
static void batlow_work_func(struct work_struct *work)
{
        int ret;

        BAT_DBG("%s enter\n", __func__);
        //notify battery low event
        ret = asus_battery_low_event();
        if (ret) {
                BAT_DBG("%s: battery low event error. %d \n", __func__, ret);
        }
}

static void batlow_enable_irq(
                struct asus_batgpio_set *p_batlow_gp,
                struct battery_low_config *p_batlow
)
{
        if (!p_batlow_gp || !p_batlow || !p_batlow_gp->active)
                return;

        if (p_batlow_gp->active) {
                pmc_enable_wakeup_event(
                        p_batlow->wake_up_event_en_bit, 
                        p_batlow->wake_up_event_type
                );
        }
}

static void batlow_disable_irq(
                struct asus_batgpio_set *p_batlow_gp,
                struct battery_low_config *p_batlow
)
{
        if (!p_batlow_gp || !p_batlow || !p_batlow_gp->active)
                return;

        if (p_batlow_gp->active) {
                pmc_disable_wakeup_event(p_batlow->wake_up_event_en_bit);
        }
}

static void batlow_irq_callback(void *data)
{
        if (batlow_wq)
                queue_work(batlow_wq, &batlow_work);
}
#endif //defined(CONFIG_ASUS_BAT_LOW_EVENT) && CONFIG_ASUS_BAT_LOW_EVENT

static int bq27520_i2c_txsubcmd(struct i2c_client *client, 
                u8 reg, unsigned short subcmd)
{
	struct i2c_msg msg;
	unsigned char data[3];

	if (!client || !client->adapter)
		return -ENODEV;

	memset(data, 0, sizeof(data));
	data[0] = reg;
	data[1] = subcmd & 0x00FF;
	data[2] = (subcmd & 0xFF00) >> 8;

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = 3;
	msg.buf = data;

	if (i2c_transfer(client->adapter, &msg, 1) < 0)
		return -EIO;

	return 0;
}

int bq27520_write_i2c(struct i2c_client *client, 
                u8 reg, int value, int b_single)
{
	struct i2c_msg msg;
	unsigned char data[3];

	if (!client || !client->adapter)
		return -ENODEV;

	memset(data, 0, sizeof(data));
	data[0] = reg;
	data[1] = value& 0x00FF;
	data[2] = (value & 0xFF00) >> 8;

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = b_single ? 2 : 3;
	msg.buf = data;

	if (i2c_transfer(client->adapter, &msg, 1) < 0)
		return -EIO;

	return 0;
}

int bq27520_read_i2c(struct i2c_client *client, 
                u8 reg, int *rt_value, int b_single)
{
	struct i2c_msg msg[1];
	unsigned char data[2];
	int err;

	if (!client || !client->adapter)
                return -ENODEV;

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 1;
	msg->buf = data;

	data[0] = reg;
	err = i2c_transfer(client->adapter, msg, 1);

	if (err >= 0) {
		if (!b_single)
			msg->len = 2;
		else
			msg->len = 1;

		msg->flags = I2C_M_RD; 
		err = i2c_transfer(client->adapter, msg, 1);
		if (err >= 0) {
			if (!b_single)
				*rt_value = get_unaligned_le16(data);
			else
				*rt_value = data[0];

			return 0;
		}
	}
	return err;
}

static inline int bq27520_read(struct i2c_client *client, 
                u8 reg, int *rt_value, int b_single)
	
{
	return bq27520_read_i2c(client, reg, rt_value, b_single);
}

static int bq27520_cntl_cmd(struct i2c_client *client, 
                u16 sub_cmd)
{
	return bq27520_i2c_txsubcmd(client, BQ27520_REG_CNTL, sub_cmd);
}

int bq27520_send_subcmd(struct i2c_client *client, int *rt_value, u16 sub_cmd)
{
	int ret, tmp_buf = 0;

        ret = bq27520_cntl_cmd(client, sub_cmd);
        if (ret) {
                dev_err(&client->dev, "Send subcommand 0x%04X error.\n", sub_cmd);
                return ret;
        }
        udelay(200);

        if (!rt_value) return ret;

        //need read data to rt_value
        ret = bq27520_read(client, BQ27520_REG_CNTL, &tmp_buf, 0);
        if (ret) 
                dev_err(&client->dev, "Error!! %s subcommand %04X\n", 
                                __func__, sub_cmd);
        *rt_value = tmp_buf;
        return ret;
}

int bq27520_cmp_i2c(int reg_off, int value)
{
        int retry = 3;
        int val=0;
        int ret=0;
        struct i2c_client *i2c = NULL;

        mutex_lock(&batt_dev_info_mutex);    
        i2c = batt_dev_info.i2c;
        mutex_unlock(&batt_dev_info_mutex);

        BAT_DBG_E("[%s] enter \n", __func__);

        while (retry--) {
                ret = bq27520_read_i2c(i2c, reg_off, &val, 1);
                if (ret < 0) continue;

                break;
        };
        if (!retry && ret < 0) {
                return ret;
        }

        return val == value ? PROC_TRUE : PROC_FALSE;
}

/*
 * Return the battery Relative State-of-Charge
 * Or < 0 if something fails.
 */
static int bq27520_battery_rsoc(struct i2c_client *client)
{
	int ret, rsoc = 0;

	ret = bq27520_read_i2c(client, BQ27520_REG_SOC, &rsoc, 0);

	if (ret) 
		return ret;

	return rsoc;
}

int bq27520_asus_battery_dev_read_percentage(void)
{
        struct i2c_client *client = NULL;
        int ret;

        mutex_lock(&batt_dev_info_mutex);    
        client = batt_dev_info.i2c; 

        ret = bq27520_battery_rsoc(client);
        if (ret < 0) {
                mutex_unlock(&batt_dev_info_mutex);
		dev_err(&client->dev,"Reading battery percentage error %d.\n", ret);
                return ret;
        }
        mutex_unlock(&batt_dev_info_mutex);

        return ret;
}

int bq27520_asus_battery_dev_read_current(void)
{
        int ret;
        int curr; 
        struct i2c_client *client = NULL;

        curr=0;

        mutex_lock(&batt_dev_info_mutex);    
        client = batt_dev_info.i2c; 

	ret = bq27520_read_i2c(client, BQ27520_REG_AI, &curr, 0);
        if (ret) {
                mutex_unlock(&batt_dev_info_mutex);
                dev_err(&client->dev, "Read current error %d.\n", ret); 
                return ret;
        }
        //dev_info(&client->dev, ":: current %04X\n", curr);

        if (curr & BIT15) 
                curr |= 0xFFFF0000; //neg. sign extend.

        curr += 0x10000; //add a base to be positive number.

        mutex_unlock(&batt_dev_info_mutex);

        return curr;
}

int bq27520_asus_battery_dev_read_volt(void)
{
        int ret;
        int volt=0;
        struct i2c_client *client = NULL;

        mutex_lock(&batt_dev_info_mutex);    
        client = batt_dev_info.i2c; 

        ret = bq27520_read_i2c(client, BQ27520_REG_VOLT, &volt, 0);
        if (ret) {
                mutex_unlock(&batt_dev_info_mutex);
                dev_err(&client->dev, "Read voltage error %d.\n", ret); 
                return ret;
        }
        //dev_info(&client->dev, ":: volt %04X\n", volt);
        mutex_unlock(&batt_dev_info_mutex);

        return volt;
}

int bq27520_asus_battery_dev_read_av_energy(void)
{
        int ret;
        struct i2c_client *client = NULL;
        int mWhr=0;

        mutex_lock(&batt_dev_info_mutex);    
        client = batt_dev_info.i2c; 

	ret = bq27520_read_i2c(client, BQ27520_REG_AE, &mWhr, 0);
        if (ret) {
                mutex_unlock(&batt_dev_info_mutex);
                dev_err(&client->dev, "Read available energy error %d.\n", ret); 
                return ret;
        }
        //dev_info(&client->dev, ":: av power %04X\n", mWhr);
        mutex_unlock(&batt_dev_info_mutex);
        return mWhr;
}

int bq27520_asus_battery_dev_read_temp(void)
{
        int ret;
        struct i2c_client *client = NULL;
        int temp=0;

        mutex_lock(&batt_dev_info_mutex);    
        client = batt_dev_info.i2c; 

	ret = bq27520_read(client, BQ27520_REG_TEMP, &temp, 0);
        if (ret) {
                mutex_unlock(&batt_dev_info_mutex);
                dev_err(&client->dev, "Read temperature error %d.\n", ret); 
                return ret;
        }
//        dev_info(&client->dev, ":: temperature %04X\n", temp);

        //tenths of K -> tenths of degree Celsius
        temp -= 2730;
        mutex_unlock(&batt_dev_info_mutex);

        return temp;
}

int bq27520_asus_battery_dev_read_remaining_capacity(void)
{
        int ret;
        struct i2c_client *client = NULL;
        int mAhr=0;

        mutex_lock(&batt_dev_info_mutex);    
        client = batt_dev_info.i2c; 

	ret = bq27520_read_i2c(client, BQ27520_REG_RM, &mAhr, 0);
        if (ret) {
                mutex_unlock(&batt_dev_info_mutex);
                dev_err(&client->dev, "Read remaining capacity error %d.\n", ret); 
                return ret;
        }
        //dev_info(&client->dev, ":: av power %04X\n", mWhr);
        mutex_unlock(&batt_dev_info_mutex);
        return mAhr;
}

int bq27520_asus_battery_dev_read_full_charge_capacity(void)
{
        int ret;
        struct i2c_client *client = NULL;
        int mAhr=0;

        mutex_lock(&batt_dev_info_mutex);    
        client = batt_dev_info.i2c; 

	ret = bq27520_read_i2c(client, BQ27520_REG_FCC, &mAhr, 0);
        if (ret) {
                mutex_unlock(&batt_dev_info_mutex);
                dev_err(&client->dev, "Read full charge capacity error %d.\n", ret); 
                return ret;
        }
        //dev_info(&client->dev, ":: av power %04X\n", mWhr);
        mutex_unlock(&batt_dev_info_mutex);
        return mAhr;
}

int bq27520_asus_battery_dev_read_chemical_id(void)
{
        struct i2c_client *client = NULL;
        int chem_id=0;
        int ret;

        mutex_lock(&batt_dev_info_mutex);    
        client = batt_dev_info.i2c; 

	ret = bq27520_send_subcmd(client, &chem_id, BQ27520_SUBCMD_CHEM_ID);
        if (ret) {
                mutex_unlock(&batt_dev_info_mutex);
                dev_err(&client->dev, "Read chemical ID error %d.\n", ret); 
                return ret;
        }
        mutex_unlock(&batt_dev_info_mutex);

        return chem_id;
}

int bq27520_asus_battery_dev_read_fw_cfg_version(void)
{
        struct i2c_client *client = NULL;
        int fw_cfg_ver=0;
        int ret;

        mutex_lock(&batt_dev_info_mutex);    
        client = batt_dev_info.i2c; 

        ret = bq27520_write_i2c(client, 0x3F, 0x01, 1);
        if (ret) {
                dev_err(&client->dev, "Get fw cfg version error %d.\n", ret); 
                return ret;
        }

        udelay(800); //delay awhile to get version. Otherwise version data not transfer complete
        ret = bq27520_read_i2c(client, 0x40, &fw_cfg_ver, 0);
        if (ret) {
                mutex_unlock(&batt_dev_info_mutex);
                dev_err(&client->dev, "Read fw cfg version error %d.\n", ret); 
                return ret;
        }
        mutex_unlock(&batt_dev_info_mutex);

        return fw_cfg_ver;
}

int __devexit bq27520_bat_i2c_remove(struct i2c_client *i2c)
{
        dev_info(&i2c->dev, "%s\n", __func__);
        return 0;
}

void __devexit bq27520_bat_i2c_shutdown(struct i2c_client *i2c)
{
        dev_info(&i2c->dev, "%s\n", __func__);
#if defined(CONFIG_ASUS_BAT_LOW_EVENT) && CONFIG_ASUS_BAT_LOW_EVENT
        batlow_disable_irq(&batlow_gp, &batlow); 
#endif
        cancel_work_sync(&batlow_work);
        asus_battery_exit();
}

#ifdef CONFIG_PM
int bq27520_bat_i2c_suspend(struct i2c_client *i2c, pm_message_t message)
{
        dev_info(&i2c->dev, "%s\n", __func__);
	asus_battery_suspend();
        return 0;
}

int bq27520_bat_i2c_resume(struct i2c_client *i2c)
{
        dev_info(&i2c->dev, "%s\n", __func__);
	asus_battery_resume();
        return 0;
}

#else
#define bq27520_bat_i2c_suspend NULL
#define bq27520_bat_i2c_resume  NULL
#endif

static int bq27520_chip_config(struct i2c_client *client)
{
	int flags = 0, ret = 0;

        ret = bq27520_send_subcmd(client, &flags, BQ27520_SUBCMD_CTNL_STATUS);
        if (ret) 
                return ret;

        dev_info(&client->dev, "bq27520 flags: 0x%04X\n", flags);

        /*
         *  No need to send this command, battery will handle it.
        ret = bq27520_send_subcmd(client, NULL, BQ27520_SUBCMD_ENABLE_IT);
        if (ret)
                return ret;
        */

//	if (!(flags & BQ27520_CS_DLOGEN)) {
//		bq27520_send_subcmd(client, NULL, BQ27520_SUBCMD_ENABLE_DLOG);
//	}

	return 0;
}

static void bq27520_hw_config(struct i2c_client *client)
{
	int ret = 0;

	ret = bq27520_chip_config(client);
	if (ret) {
		dev_err(&client->dev, "Failed to config Bq27520 ret = %d\n", ret);
		return;
	}
}

struct info_entry {
        char name[30];
        u16 cmd;
};

static struct info_entry dump_subcmd_info_tbl[] = {
        {"control status", BQ27520_SUBCMD_CTNL_STATUS},
        {"device type", BQ27520_SUBCMD_DEVICE_TYPE},
        {"firmware version", BQ27520_SUBCMD_FW_VER},
        {"chemical id", BQ27520_SUBCMD_CHEM_ID},
};

static struct info_entry dump_info_tbl[] = {
        {"Temperature", BQ27520_REG_TEMP},
        {"Voltage", BQ27520_REG_VOLT},
        {"Flags", BQ27520_REG_FLAGS},
        {"RemainingCapacity", BQ27520_REG_RM},
        {"AverageCurrent", BQ27520_REG_AI},
        {"AvailableEnergy", BQ27520_REG_AE},
};
        
static void bq27520_dump_info(struct i2c_client *client)
{
        u32 i;

        for (i=0; i<sizeof(dump_subcmd_info_tbl)/sizeof(struct info_entry); i++){
                int tmp_buf=0, ret=0;

                ret = bq27520_send_subcmd(client, 
                        &tmp_buf, dump_subcmd_info_tbl[i].cmd);
                if (ret) continue;

                dev_info(&client->dev, "%s, 0x%04X\n", dump_subcmd_info_tbl[i].name, tmp_buf);
        }

        for (i=0; i<sizeof(dump_info_tbl)/sizeof(struct info_entry); i++){
                int tmp_buf=0, ret=0;

                ret = bq27520_read(client, 
                        dump_info_tbl[i].cmd, &tmp_buf, 0);
                if (ret) continue;

                dev_info(&client->dev, "%s, 0x%04X\n", dump_info_tbl[i].name, tmp_buf);
        }
}



u32 read_reg(u32 addr)
{
        u32 tmp;

        if (!(addr & (0x4-1))) { //align 4byte
                tmp = *(volatile u32 *)(addr);
        }else if (!(addr & (0x2-1))) {
                tmp = *(volatile u16 *)(addr);
        }else {
                tmp = *(volatile u8 *)(addr); 
        }

        return tmp;
}

void write_reg(u32 addr, u32 clear_bit, u32 set_bit)
{

        if (!(addr & (0x4-1))) { //align 4byte
                REG32_VAL(addr) &= ~clear_bit; 
                REG32_VAL(addr) |= set_bit;
        }else if (!(addr & (0x2-1))) {
                REG16_VAL(addr) &= ~clear_bit; 
                REG16_VAL(addr) |= set_bit;
        }else {
                REG8_VAL(addr) &= ~clear_bit; 
                REG8_VAL(addr) |= set_bit;
        }
        BAT_DBG("write 0x%08X, 0x%08X, 0x%08X, rd 0x%08X", 
                addr, clear_bit, set_bit, read_reg(addr));
}

#if defined(CONFIG_ASUS_BAT_LOW_EVENT) && CONFIG_ASUS_BAT_LOW_EVENT
int bq27520_batlow_config(
                struct i2c_client *client, 
                struct asus_batgpio_set *p_batlow_gp, 
                struct battery_low_config *p_bat_config
) 
{
        int ret=0;
        u32 wake_up_event_en_bit=0;
        u32 wake_up_event_type=0;

        if (!p_batlow_gp || !p_bat_config) { 
                BAT_DBG_E("(%s) Skipping ... \n", __func__);
                return -EINVAL;
        }

        //config gpio
        if (!p_batlow_gp->active) {
                BAT_DBG_E("(%s) Not active ... \n", __func__);
                return -EINVAL;
        }

        write_reg(p_batlow_gp->ctraddr, 0, p_batlow_gp->bitmap);
        write_reg(p_batlow_gp->icaddr, p_batlow_gp->bitmap, 0); //config GPI
        /*
        write_reg(p_batlow_gp->ipcaddr, 0, p_batlow_gp->bitmap); //pull up/down enable
        if (p_batlow_gp->active & 0x1){
                write_reg(p_gpio->ipdaddr, 0, p_gpio->bitmap);
        }else{
                write_reg(p_gpio->ipdaddr, p_gpio->bitmap, 0);
        }
        */

        wake_up_event_en_bit = p_bat_config->wake_up_event_en_bit;
        wake_up_event_type = p_bat_config->wake_up_event_type;

	ret = pmc_register_callback(wake_up_event_en_bit, batlow_irq_callback, client);
        if (ret) {
                BAT_DBG_E("(%s) register pmc callback error %d, No battery low interrupt handler...\n", __func__, ret);
                return ret;
        } 

        batlow_enable_irq(p_batlow_gp, p_bat_config);

        return 0;
}
#endif //defined(CONFIG_ASUS_BAT_LOW_EVENT) && CONFIG_ASUS_BAT_LOW_EVENT

int bq27520_batt_sel_config(struct asus_batgpio_set *p_gpio) 
{
        if (!p_gpio->active)
                return 0;

        //config gpio
        write_reg(p_gpio->ctraddr, 0, p_gpio->bitmap);
        write_reg(p_gpio->icaddr, p_gpio->bitmap, 0); //config GPI

        return 0;
}

int bq27520_batt_current_sel_type(void) 
{
        /* goto here MUST after load gpio address */

        if (!gpio_cell_selector.idaddr ) {
                BAT_DBG_E("(%s) GPIO address not ready.\n", __func__);
                return -EINVAL;
        }

        return ((read_reg(gpio_cell_selector.idaddr) & BIT5)>>5);
}

int bq27520_batt_fw_sel_type(void) 
{
        int ret_val = 0;
        int cell_type = 0;

        ret_val = bq27520_asus_battery_dev_read_chemical_id();
        if (ret_val < 0) {
                BAT_DBG_E("[%s] Fail to get chemical_id\n", __func__);
                return -EINVAL;
        }
        
        if (ret_val == 0x355) {
                cell_type = TYPE_LG;

        } else if (ret_val == 0x133) {
                cell_type = TYPE_COS_LIGHT;

        } else {
                BAT_DBG_E("[%s] wrong chemical_id 0x%04X\n", __func__, ret_val);
                return -EINVAL;
        }

        return cell_type;

}

int bq27520_is_normal_mode() 
{
        int retry = RETRY_COUNT;
        struct i2c_client *i2c = NULL;
        int val=0;
        int ret=0;

        BAT_DBG_E("[%s] enter \n", __func__);

        mutex_lock(&batt_dev_info_mutex);    
        i2c = batt_dev_info.i2c; 
        mutex_unlock(&batt_dev_info_mutex);

        while (retry--) {
                ret = bq27520_read_i2c(i2c, 0x00, &val, 1);
                if (ret < 0) continue;

                break;
        };
        if (ret < 0) {
                return 0; //not normal mode
        }
        return 1; //it's normal mode

}

static int __devinit 
bq27520_bat_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
{
        int ret = 0;
        int retry=0;
        int status = 0;

        mutex_lock(&batt_dev_info_mutex);    
        batt_dev_info.status = DEV_INIT;
        mutex_unlock(&batt_dev_info_mutex);

#if defined(CONFIG_ASUS_ENGINEER_MODE) && CONFIG_ASUS_ENGINEER_MODE
        ret = bq27520_bat_upt_main_update_flow(); 
        mutex_lock(&batt_dev_info_mutex);    
        batt_dev_info.update_status = ret;
        mutex_unlock(&batt_dev_info_mutex);
        if (ret < 0) 
                goto update_fail;
        //force sealed if needed.
        retry = 5;
        do {
                //read status 
        	ret = bq27520_send_subcmd(i2c, &status, BQ27520_SUBCMD_CTNL_STATUS);
                if (ret < 0) {
                        dev_err(&i2c->dev, "Read status error %d.\n", ret); 
                        continue;
                }
                //check sealed state
                if (status & (BIT5<<8)) {
                        break; //already sealed
                }
                //send sealed again
                dev_info(&i2c->dev, "Not sealed detected, send sealed again ...\n");
                ret = bq27520_send_subcmd(i2c, &status, BQ27520_SUBCMD_SEALED);
                if (ret < 0) {
                        dev_err(&i2c->dev, "Send seal command FAIL (%d)\n", ret);
                }

                msleep(20);
        } while (--retry);
#endif

        bq27520_hw_config(i2c);

        bq27520_dump_info(i2c);

        dev_info(&i2c->dev, "%s init done.", __func__);

        bq27520_tbl.read_percentage = bq27520_asus_battery_dev_read_percentage;
        bq27520_tbl.read_current = bq27520_asus_battery_dev_read_current;
        bq27520_tbl.read_volt = bq27520_asus_battery_dev_read_volt;
        bq27520_tbl.read_av_energy = bq27520_asus_battery_dev_read_av_energy;
        bq27520_tbl.read_temp = bq27520_asus_battery_dev_read_temp;
        ret = asus_register_power_supply(&i2c->dev, &bq27520_tbl);
        if (ret)
                goto power_register_fail;
#if defined(CONFIG_ASUS_BAT_LOW_EVENT) && CONFIG_ASUS_BAT_LOW_EVENT
        /* create & init battery_low work Q */
        batlow_wq = create_singlethread_workqueue("asus_battery_low");
        if(!batlow_wq) 
        {
                BAT_DBG_E("Create battery_low  thread fail");
                ret = -ENOMEM;
                goto error_workq;
        }
        INIT_WORK(&batlow_work, batlow_work_func);

        /* battery low config */
        ret = bq27520_batlow_config(i2c, &batlow_gp, &batlow);
        if (ret) {
                BAT_DBG_E("battery low config fail.\n");
                ret = 0;
        }
#endif

        mutex_lock(&batt_dev_info_mutex);    
        batt_dev_info.status = DEV_INIT_OK;
        mutex_unlock(&batt_dev_info_mutex);

        dev_info(&i2c->dev, "register done. %s", __func__);

power_register_fail:
#if defined(CONFIG_ASUS_ENGINEER_MODE) && CONFIG_ASUS_ENGINEER_MODE
update_fail:
#endif
        return ret;
}

static struct bq27520_bat_platform_data bq27520_bat_pdata = {
        .i2c_bus_id      = BQ27520_I2C_BUS,
};

static struct i2c_board_info bq27520_bat_i2c_board_info = {
        .type          = BQ27520_DEV_NAME,
        .flags         = 0x00,
        .addr          = BQ27520_I2C_DEFAULT_ADDR,
        .archdata      = NULL,
        .irq           = -1,
        .platform_data = &bq27520_bat_pdata,
};

static struct i2c_device_id bq27520_bat_i2c_id[] = {
	{ BQ27520_DEV_NAME, 0 },
	{ },
};

static struct i2c_driver bq27520_bat_i2c_driver = {
        .driver    = {
                .name  = BQ27520_DEV_NAME,
                .owner = THIS_MODULE,
        },
        .probe     = bq27520_bat_i2c_probe,
        .remove    = bq27520_bat_i2c_remove,
        .shutdown  = bq27520_bat_i2c_shutdown,
        .suspend   = bq27520_bat_i2c_suspend,
        .resume    = bq27520_bat_i2c_resume,
        .id_table  = bq27520_bat_i2c_id,
};

static char uboot_var_buf[800];
static int uboot_var_buf_size = sizeof(uboot_var_buf); 
int bq27520_load_bat_config(
        struct asus_bat_config *p_bat_cfg, 
        struct asus_batgpio_set *p_batlow_gp,
        struct battery_low_config *p_batlow,
        struct asus_batgpio_set *p_cell_sel,
        u32 *p_test_flag
) 
{
	char varname[] = "asus.io.bat";
	char varname_batlow_config[] = "asus.bl.bat";
	char varname_gpio[] = "asus.gp_bl.bat";
	char varname_gpio_cell_sel[] = "asus.gp_sel.bat";
	char varname_debug[] = "asus.debug.bat";
        int allow_batlow = 1;

        if (!p_bat_cfg || !p_batlow || !p_test_flag) {
                return -EINVAL;
        }

	if (!wmt_getsyspara(varname, uboot_var_buf, &uboot_var_buf_size) ){
                sscanf(uboot_var_buf,"%d:%d:%d:%x",
                                &p_bat_cfg->battery_support, 
                                &p_bat_cfg->polling_time, 
                                &p_bat_cfg->critical_polling_time,
                                &p_bat_cfg->slave_addr
                        );
        } else {
                //Oops!! Read uboot parameter fail. Use default instead
                //bq27520 default NOT support.
                p_bat_cfg->battery_support = 1;
                p_bat_cfg->polling_time = BATTERY_DEFAULT_POLL_TIME;
                p_bat_cfg->critical_polling_time = BATTERY_CRITICAL_POLL_TIME;
                p_bat_cfg->slave_addr = BQ27520_I2C_DEFAULT_ADDR;
                
                allow_batlow = 0;
        }

        if (allow_batlow && !wmt_getsyspara(varname_gpio, uboot_var_buf, &uboot_var_buf_size) ){
                sscanf(uboot_var_buf, "%d:%x:%x:%x:%x:%x:%x",
                                &p_batlow_gp->active,
                                &p_batlow_gp->bitmap,
                                &p_batlow_gp->ctraddr,
                                &p_batlow_gp->icaddr,
                                &p_batlow_gp->idaddr,
                                &p_batlow_gp->ipcaddr,
                                &p_batlow_gp->ipdaddr
                      );
        } else {
                BAT_DBG_E("[bq27520] warning: asus.gp.bat use default value.");
                p_batlow_gp->active = 0;
                p_batlow_gp->bitmap = 0x01; //enable battery low gpio pin
                p_batlow_gp->ctraddr = 0xd8110042;
                p_batlow_gp->icaddr = 0xd8110082;
                p_batlow_gp->idaddr = 0xd8110000;
                p_batlow_gp->ipcaddr = 0xd8110482;
                p_batlow_gp->ipdaddr = 0xd81104c0;
                p_batlow_gp->int_active = 0; //interrupt not support
        }

                                
	if (allow_batlow && !wmt_getsyspara(varname_batlow_config, uboot_var_buf, &uboot_var_buf_size) ){
                sscanf(uboot_var_buf, "%x:%d", 
                                &p_batlow->wake_up_event_en_bit,
                                &p_batlow->wake_up_event_type
                                );
        } else {
                BAT_DBG_E("[bq27520] warning: asus.bf.bat use default value.");
                p_batlow->wake_up_event_en_bit = WAKEUP0_EVENT_ID;
                p_batlow->wake_up_event_type = WAKE_UP_RISING_EDGE;
        }

	if (!wmt_getsyspara(varname_gpio_cell_sel, uboot_var_buf, &uboot_var_buf_size) ){
                sscanf(uboot_var_buf, "%d:%x:%x:%x:%x:%x:%x:%d:%x:%x:%x:%x:%x:%x", 
                                &p_cell_sel->active, 
                                &p_cell_sel->bitmap,
                                &p_cell_sel->ctraddr,
                                &p_cell_sel->icaddr,
                                &p_cell_sel->idaddr,
                                &p_cell_sel->ipcaddr,
                                &p_cell_sel->ipdaddr,
                                &p_cell_sel->int_active,
                                &p_cell_sel->int_bitmap,
                                &p_cell_sel->int_sts_addr,
                                &p_cell_sel->int_ctrl_addr,
                                &p_cell_sel->int_type_addr,
                                &p_cell_sel->int_type_val_clr,
                                &p_cell_sel->int_type_val_set
                                );
        } else {
                BAT_DBG_E("[bq27520] warning: asus.gp_sel.bat use default value.");
                p_cell_sel->active = 0; 
                p_cell_sel->bitmap = BIT5; //enable cell selector gpio pin
                p_cell_sel->ctraddr = 0xd8110040;
                p_cell_sel->icaddr = 0xd8110080;
                p_cell_sel->idaddr = 0xd8110000;
                p_cell_sel->ipcaddr = 0xd8110480;
                p_cell_sel->ipdaddr = 0xd81104c0;
                p_cell_sel->int_active = -1; //interrupt not support 
        }

	if (!wmt_getsyspara(varname_debug, uboot_var_buf, &uboot_var_buf_size) ) {
                sscanf(uboot_var_buf, "%x", p_test_flag);
        } else {
                *p_test_flag = 0;
        }

        /* gpio base checking for uncorrect variable */
	p_cell_sel->ctraddr += WMT_MMAP_OFFSET;
	p_cell_sel->icaddr += WMT_MMAP_OFFSET;
	p_cell_sel->idaddr += WMT_MMAP_OFFSET;
	p_cell_sel->ipcaddr += WMT_MMAP_OFFSET;
	p_cell_sel->ipdaddr += WMT_MMAP_OFFSET;
        if (p_cell_sel->ctraddr < CONFIG_PAGE_OFFSET ||
                p_cell_sel->icaddr < CONFIG_PAGE_OFFSET || 
                p_cell_sel->idaddr < CONFIG_PAGE_OFFSET ||
                p_cell_sel->ipcaddr < CONFIG_PAGE_OFFSET ||
                p_cell_sel->ipdaddr < CONFIG_PAGE_OFFSET) {

                BAT_DBG_E("[bq27520] cell type: GPIO base not correct.\n");
                p_cell_sel->active = 0;
        }

        p_batlow_gp->ctraddr += WMT_MMAP_OFFSET;
        p_batlow_gp->icaddr += WMT_MMAP_OFFSET;
        p_batlow_gp->idaddr += WMT_MMAP_OFFSET;
        p_batlow_gp->ipcaddr += WMT_MMAP_OFFSET;
        p_batlow_gp->ipdaddr += WMT_MMAP_OFFSET;
        p_batlow_gp->int_ctrl_addr += WMT_MMAP_OFFSET;
        p_batlow_gp->int_sts_addr += WMT_MMAP_OFFSET;
        p_batlow_gp->int_type_addr += WMT_MMAP_OFFSET;

        if (p_batlow_gp->ctraddr < CONFIG_PAGE_OFFSET ||
                        p_batlow_gp->icaddr < CONFIG_PAGE_OFFSET ||
                        p_batlow_gp->idaddr < CONFIG_PAGE_OFFSET ||
                        p_batlow_gp->ipcaddr < CONFIG_PAGE_OFFSET ||
                        p_batlow_gp->ipdaddr < CONFIG_PAGE_OFFSET) {
                BAT_DBG_E("[bq27520] batlow: GPIO base not correct.\n");
                p_batlow_gp->active = 0;
                                }

        return 0;
}

static int __init bq27520_bat_i2c_init(void)
{
        int ret = 0;
        struct i2c_adapter *adapter = NULL;
        struct i2c_client *client   = NULL;
        struct i2c_board_info *board_info= NULL;
        struct asus_bat_config bat_cfg;
        u32 test_flag=0;
        u32 test_major_flag, test_minor_flag;
        int cell_sel=0;

        /* load uboot config */
        ret = bq27520_load_bat_config(&bat_cfg, &batlow_gp, &batlow, &gpio_cell_selector, &test_flag);
        if (ret) {
                BAT_DBG_E("load battery config fail.\n");
                return ret;
        }

        /* GPIO config */
        ret = bq27520_batt_sel_config(&gpio_cell_selector);
        if (ret) {
                BAT_DBG_E("Battery selector config fail.\n");
                return ret;
        }
        
        //if fw sel type is not equal, it will enter update process later in probe function
        cell_sel = bq27520_batt_current_sel_type(); 
        if (cell_sel < 0) {
                BAT_DBG_E("Read battery selector fail.\n");
                return cell_sel;
        }
        BAT_DBG_E("current cell status = %d\n", cell_sel);


        test_major_flag = test_flag & 0x0FFFF;
        test_minor_flag = test_flag >> 16;
        if (test_major_flag & TEST_INFO_NO_REG_POWER)
                test_minor_flag = BIT0;

        if (!bat_cfg.battery_support) {
                BAT_DBG_E("Battery not support. Skip ... \n");
                ret = -ENODEV;
                goto battery_not_support;
        }

        if (bat_cfg.slave_addr != BQ27520_I2C_DEFAULT_ADDR && 
                  bat_cfg.slave_addr != BATTERY_I2C_ANY_ADDR) {
                BAT_DBG_E("Not select %s driver. Skip ... \n", 
                                bq27520_bat_i2c_driver.driver.name );
                ret = -ENODEV;
                goto battery_not_support;
        }

        //turn to jiffeys/s 
        bat_cfg.polling_time *= HZ;
        bat_cfg.critical_polling_time *= HZ;

        //register i2c device
        board_info = &bq27520_bat_i2c_board_info;
        adapter = i2c_get_adapter(((struct bq27520_bat_platform_data *)board_info->platform_data)->i2c_bus_id);
        if (adapter == NULL) {
                BAT_DBG_E("can not get i2c adapter, client address error\n");
                ret = -ENODEV;
                goto get_adapter_fail;
        }

        client = i2c_new_device(adapter, board_info);
        if (client == NULL) {
                BAT_DBG("allocate i2c client failed\n");
                ret = -ENOMEM;
                goto register_i2c_device_fail;
        }
        i2c_put_adapter(adapter);

        mutex_lock(&batt_dev_info_mutex);    
        batt_dev_info.i2c = client;
        batt_dev_info.test_flag = test_minor_flag;
        mutex_unlock(&batt_dev_info_mutex);

#if defined(CONFIG_ASUS_ENGINEER_MODE) && CONFIG_ASUS_ENGINEER_MODE
        ret = bq27520_bat_upt_i2c_init(); 
        if (ret) 
                goto bq27520_upt_i2c_init_fail;
#endif


#if CONFIG_PROC_FS
        ret = bq27520_register_proc_fs();
        if (ret) {
                BAT_DBG_E("Unable to create proc file\n");
                goto proc_fail;
        }
#endif
        //init battery info & work queue
        ret = asus_battery_init(bat_cfg.polling_time, bat_cfg.critical_polling_time, cell_sel, test_major_flag); //patch
        if (ret)
                goto asus_battery_init_fail;

        //register i2c driver
        ret =  i2c_add_driver(&bq27520_bat_i2c_driver);
        if (ret) {
                BAT_DBG("register bq27520 battery i2c driver failed\n");
                goto i2c_register_driver_fail;
        }

        return ret;

i2c_register_driver_fail:
        asus_battery_exit();
asus_battery_init_fail:
#if defined(CONFIG_ASUS_ENGINEER_MODE) && CONFIG_ASUS_ENGINEER_MODE
bq27520_upt_i2c_init_fail:
#endif
#if CONFIG_PROC_FS
proc_fail:
#endif
        i2c_unregister_device(client);
register_i2c_device_fail:
get_adapter_fail:
battery_not_support:
        return ret;
}
late_initcall(bq27520_bat_i2c_init);

static void __exit bq27520_bat_i2c_exit(void)
{
        struct i2c_client *client = NULL;
        
        asus_battery_exit();

        mutex_lock(&batt_dev_info_mutex);    
        client = batt_dev_info.i2c;
        mutex_unlock(&batt_dev_info_mutex);

#if defined(CONFIG_ASUS_ENGINEER_MODE) && CONFIG_ASUS_ENGINEER_MODE
        bq27520_bat_upt_i2c_exit();
#endif
        i2c_unregister_device(batt_dev_info.i2c);
        i2c_del_driver(&bq27520_bat_i2c_driver);

        BAT_DBG("%s exit\n", __func__);
}
module_exit(bq27520_bat_i2c_exit);

MODULE_AUTHOR("Wade1_Chen@asus.com");
MODULE_DESCRIPTION("battery bq27520 driver");
MODULE_LICENSE("GPL");
