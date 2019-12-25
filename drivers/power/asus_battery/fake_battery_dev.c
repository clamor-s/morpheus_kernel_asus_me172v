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
#include <linux/uaccess.h>
#include <linux/proc_fs.h>

#include "asus_battery.h"

struct fake_dev_info_reply {
        u32 batt_volt;			/* Battery voltage */
        int batt_current;		/* Battery current */
        int percentage;			/* Battery percentage, < 0 is unknown status */
        int batt_temp;			/* Battery Temperature */
        int batt_av_energy;
} fake_bat;

//extern function 
extern int wmt_getsyspara(char *varname, char *varval, int *varlen);
//

static struct asus_batgpio_set  batlow ;
static struct workqueue_struct *batlow_wq;
static struct work_struct batlow_work;
static struct dev_func fake_tbl;

struct battery_dev_info fake_batt_dev_info;
DEFINE_MUTEX(fake_batt_dev_info_mutex);

int fake_batt_proc_read(char *page, char **start, off_t off, int count, int *eof, void *date)
{
    int len;
    struct fake_dev_info_reply tmp_batt_info;

    BAT_DBG_E("%s\n", __func__);

    mutex_lock(&fake_batt_dev_info_mutex);
    tmp_batt_info = fake_bat;
    mutex_unlock(&fake_batt_dev_info_mutex);

    len = sprintf(page, 
            "percentage %d%%\n"
            "voltage %d\n"
            "current %d\n"
            "av_energy %d\n"
            "temperature %d\n",
            tmp_batt_info.percentage,
            tmp_batt_info.batt_volt,
            tmp_batt_info.batt_current,
            tmp_batt_info.batt_av_energy,
            tmp_batt_info.batt_temp
                );
            
    return len;
}
u32 cmd_start_flag=0;
char  proc_cmd[20];
char  proc_field[20];
char  proc_value[20];
char  proc_arg1[20];
char  proc_buf[64];
struct fake_dev_info_reply proc_bat_info;

#define BAT_CMD_START   0
#define BAT_CMD_END     1
#define BAT_CMD_SET     2
#define BAT_CMD_RESTART 3
int fake_batt_proc_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
/* <command> <field> <value> <arg1>
 * command: START, END, SET, GET
 * field: field of battery_info_reply 
 * value: field value (integer or string)
 */
    int cmd=-1; //0: start, 1: end, 2: set

    BAT_DBG_E("%s\n", __func__);

    if (count > sizeof(proc_buf)) {
        BAT_DBG_E("data error\n");
        return -EINVAL;
    }

    if (copy_from_user(proc_buf, buffer, count)) {
        BAT_DBG_E("read data from user space error\n");
        return -EFAULT;
    }

   
    sscanf(proc_buf, "%s %s %s %s\n", proc_cmd, proc_field, proc_value, proc_arg1);   
    BAT_DBG_E("%s,%s,%s,%s\n", proc_cmd, proc_field, proc_value, proc_arg1);   

   cmd = -1;
   if (!strcmp("START", proc_cmd))
       cmd = BAT_CMD_START;

   else if (!strcmp("END", proc_cmd))
       cmd = BAT_CMD_END;

   else if (!strcmp("SET", proc_cmd))
       cmd = BAT_CMD_SET;

   else if (!strcmp("RESTART", proc_cmd))
       cmd = BAT_CMD_RESTART;

   else{
       BAT_DBG_E("command not found\n");
       return -EINVAL;
   }

   if ((cmd == BAT_CMD_SET || cmd == BAT_CMD_END) && !cmd_start_flag) {
       BAT_DBG_E("command error\n");
       return -EINVAL;
   }

   //start to process command +++++++
   if (cmd == BAT_CMD_START) {
       cmd_start_flag = 1;
       mutex_lock(&fake_batt_dev_info_mutex);
       proc_bat_info = fake_bat;
       mutex_unlock(&fake_batt_dev_info_mutex);
       return count;

   } else if (cmd == BAT_CMD_END) {
       cmd_start_flag = 0;

       mutex_lock(&fake_batt_dev_info_mutex);
       fake_bat = proc_bat_info;
       mutex_unlock(&fake_batt_dev_info_mutex);

       update_battery_driver_state();

       return count;

   } else if (cmd == BAT_CMD_RESTART) {
       cmd_start_flag = 1;

       mutex_lock(&fake_batt_dev_info_mutex);
       proc_bat_info = fake_bat;
       mutex_unlock(&fake_batt_dev_info_mutex);

       return count;
   }


   if (!strcmp("percentage", proc_field)) {
       sscanf(proc_value, "%d", &proc_bat_info.percentage);
       BAT_DBG_E("percentage %d\n", proc_bat_info.percentage);

   } else if (!strcmp("batt_temp", proc_field)) {
       sscanf(proc_value, "%d", &proc_bat_info.batt_temp);
       BAT_DBG_E("batt_temp %d\n", proc_bat_info.batt_temp);

   } else if (!strcmp("batt_volt", proc_field)) {
       sscanf(proc_value, "%d", &proc_bat_info.batt_volt);
       BAT_DBG_E("batt_volt %d\n", proc_bat_info.batt_volt);

   } else if (!strcmp("batt_current", proc_field)) {
       sscanf(proc_value, "%d", &proc_bat_info.batt_current);
       BAT_DBG_E("batt_volt %d\n", proc_bat_info.batt_current);

   } else if (!strcmp("batt_av_energy", proc_field)) {
       sscanf(proc_value, "%d", &proc_bat_info.batt_av_energy);
       BAT_DBG_E("batt_volt %d\n", proc_bat_info.batt_av_energy);

   } else {
       BAT_DBG_E("field not support or error\n");
       return -EINVAL;
   }

   BAT_DBG_E("OK\n");

   return count;
}


int fake_asus_battery_dev_read_percentage(void)
{
        struct i2c_client *client = NULL;
        struct fake_dev_info_reply tmp_fake;

        mutex_lock(&fake_batt_dev_info_mutex);    
        client = fake_batt_dev_info.i2c; 
        tmp_fake = fake_bat;
        mutex_unlock(&fake_batt_dev_info_mutex);

        return tmp_fake.percentage;
}

int fake_asus_battery_dev_read_current(void)
{
        int curr; 
        struct fake_dev_info_reply tmp_fake;
        
        curr=0;

        mutex_lock(&fake_batt_dev_info_mutex);    
        tmp_fake = fake_bat;
        mutex_unlock(&fake_batt_dev_info_mutex);

        curr = tmp_fake.batt_current;
        curr += 0x10000; //add a base to be positive number.

        return curr;
}

int fake_asus_battery_dev_read_volt(void)
{
        int volt=0;
        struct fake_dev_info_reply tmp_fake;

        return tmp_fake.batt_volt;
}

int fake_asus_battery_dev_read_av_energy(void)
{
        int ret;
        struct i2c_client *client = NULL;
        int mWhr=0;
        struct fake_dev_info_reply tmp_fake;
        

        mutex_lock(&fake_batt_dev_info_mutex);    
        client = fake_batt_dev_info.i2c; 
        tmp_fake = fake_bat;
        mutex_unlock(&fake_batt_dev_info_mutex);

        return tmp_fake.batt_av_energy;
}

int fake_asus_battery_dev_read_temp(void)
{
        int ret;
        struct i2c_client *client = NULL;
        int temp=0;
        struct fake_dev_info_reply tmp_fake;

        mutex_lock(&fake_batt_dev_info_mutex);    
        client = fake_batt_dev_info.i2c; 
        tmp_fake = fake_bat;
        mutex_unlock(&fake_batt_dev_info_mutex);

        return tmp_fake.batt_temp;
}


static int __devexit fake_bat_i2c_remove(struct i2c_client *i2c)
{
    dev_info(&i2c->dev, "%s\n", __func__);
    return 0;
}

#ifdef CONFIG_PM
static int fake_bat_i2c_suspend(struct i2c_client *i2c, pm_message_t message)
{
    dev_info(&i2c->dev, "%s\n", __func__);
    return 0;
}

static int fake_bat_i2c_resume(struct i2c_client *i2c)
{
    dev_info(&i2c->dev, "%s\n", __func__);
    return 0;
}

#else
#define fake_bat_i2c_suspend NULL
#define fake_bat_i2c_resume  NULL
#endif

static void fake_batlow_work_func(struct work_struct *work)
{
        int ret;

        BAT_DBG("%s enter\n", __func__);
        //notify battery low event
        if (ret) {
                BAT_DBG("%s: battery low event error. %d \n", __func__, ret);
        }
}

static int __devinit 
fake_bat_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
{
        int ret = 0;

        mutex_lock(&fake_batt_dev_info_mutex);    
        fake_batt_dev_info.status = DEV_INIT;
        mutex_unlock(&fake_batt_dev_info_mutex);

        dev_info(&i2c->dev, "%s init done.", __func__);

        fake_tbl.read_percentage = fake_asus_battery_dev_read_percentage;
        fake_tbl.read_volt = fake_asus_battery_dev_read_current;
        fake_tbl.read_current = fake_asus_battery_dev_read_volt;
        fake_tbl.read_av_energy = fake_asus_battery_dev_read_av_energy;
        fake_tbl.read_temp = fake_asus_battery_dev_read_temp;
        ret = asus_register_power_supply(&i2c->dev, &fake_tbl);
        if (ret)
                goto power_register_fail;

        mutex_lock(&fake_batt_dev_info_mutex);    
        fake_batt_dev_info.status = DEV_INIT_OK;
        mutex_unlock(&fake_batt_dev_info_mutex);

        //gpio_irq_config(i2c);
        dev_info(&i2c->dev, "register done. %s", __func__);

power_register_fail:
        return ret;
}

#define FAKE_I2C_BUS 3
#define FAKE_DEV_NAME "FakeBat"
#define FAKE_I2C_DEFAULT_ADDR 0x7F
struct fake_bat_platform_data {
        u32 i2c_bus_id;
};
static struct fake_bat_platform_data fake_bat_pdata = {
        .i2c_bus_id      = FAKE_I2C_BUS,
};

static struct i2c_board_info fake_bat_i2c_board_info = {
        .type          = FAKE_DEV_NAME,
        .flags         = 0x00,
        .addr          = FAKE_I2C_DEFAULT_ADDR,
        .archdata      = NULL,
        .irq           = -1,
        .platform_data = &fake_bat_pdata,
};


static struct i2c_device_id fake_bat_i2c_id[] = {
	{ FAKE_DEV_NAME, 0 },
	{ },
};

static struct i2c_driver fake_bat_i2c_driver = {
        .driver    = {
                .name  = FAKE_DEV_NAME,
                .owner = THIS_MODULE,
        },
        .probe     = fake_bat_i2c_probe,
        .remove    = fake_bat_i2c_remove,
        .suspend   = fake_bat_i2c_suspend,
        .resume    = fake_bat_i2c_resume,
        .id_table  = fake_bat_i2c_id,
};

static char uboot_var_buf[800];
static int uboot_var_buf_size = sizeof(uboot_var_buf); 
int fake_load_bat_config(
        struct asus_bat_config *p_bat_cfg, 
        struct asus_batgpio_set *p_batlow,
        u32 *p_test_flag
) 
{
	char varname[] = "asus.io.bat";
	char varname_gpio[] = "asus.gp.bat";
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
                //fake battery default NOT support.
                p_bat_cfg->battery_support = 0;
                p_bat_cfg->polling_time = BATTERY_DEFAULT_POLL_TIME;
                p_bat_cfg->critical_polling_time = BATTERY_CRITICAL_POLL_TIME;
                p_bat_cfg->slave_addr = FAKE_I2C_DEFAULT_ADDR;
                
                allow_batlow = 0;
        }
                                

	if (allow_batlow && !wmt_getsyspara(varname_gpio, uboot_var_buf, &uboot_var_buf_size) ){
                sscanf(uboot_var_buf, "%d:%x:%x:%x:%x:%x:%x:%d:%x:%x:%x:%x:%x:%x", 
                                &p_batlow->active, 
                                &p_batlow->bitmap,
                                &p_batlow->ctraddr,
                                &p_batlow->icaddr,
                                &p_batlow->idaddr,
                                &p_batlow->ipcaddr,
                                &p_batlow->ipdaddr,
                                &p_batlow->int_active,
                                &p_batlow->int_bitmap,
                                &p_batlow->int_sts_addr,
                                &p_batlow->int_ctrl_addr,
                                &p_batlow->int_type_addr,
                                &p_batlow->int_type_val_clr,
                                &p_batlow->int_type_val_set
                                );
        } else {
                BAT_DBG_E("[fake] warning: asus.gp.bat use default value.");
                p_batlow->active = 0; 
                p_batlow->bitmap = 0x01; //enable battery low gpio pin
                p_batlow->ctraddr = 0xd8110042;
                p_batlow->icaddr = 0xd8110082;
                p_batlow->idaddr = 0xd8110000;
                p_batlow->ipcaddr = 0xd8110482;
                p_batlow->ipdaddr = 0xd81104c0;
                p_batlow->int_active = -1; //interrupt not support 
        }


	if (!wmt_getsyspara(varname_debug, uboot_var_buf, &uboot_var_buf_size)) {
                sscanf(uboot_var_buf, "%x", p_test_flag);
        } else {
                *p_test_flag = 0;
        }

	p_batlow->ctraddr += WMT_MMAP_OFFSET;
	p_batlow->icaddr += WMT_MMAP_OFFSET;
	p_batlow->idaddr += WMT_MMAP_OFFSET;
	p_batlow->ipcaddr += WMT_MMAP_OFFSET;
	p_batlow->ipdaddr += WMT_MMAP_OFFSET;
        p_batlow->int_ctrl_addr += WMT_MMAP_OFFSET;
        p_batlow->int_sts_addr += WMT_MMAP_OFFSET;
        p_batlow->int_type_addr += WMT_MMAP_OFFSET;

        /* gpio base checking for uncorrect variable */
        if (p_batlow->ctraddr < CONFIG_PAGE_OFFSET ||
                p_batlow->icaddr < CONFIG_PAGE_OFFSET ||
                p_batlow->idaddr < CONFIG_PAGE_OFFSET ||
                p_batlow->ipcaddr < CONFIG_PAGE_OFFSET ||
                p_batlow->ipdaddr < CONFIG_PAGE_OFFSET ||
                p_batlow->int_ctrl_addr < CONFIG_PAGE_OFFSET ||
                p_batlow->int_sts_addr < CONFIG_PAGE_OFFSET ||
                p_batlow->int_type_addr < CONFIG_PAGE_OFFSET) {

                BAT_DBG_E("[fake] GPIO base not correct.\n");
                p_batlow->active = 0;
        }

        return 0;
}

static int __init fake_bat_i2c_init(void)
{
        int ret = 0;
        struct i2c_adapter *adapter = NULL;
        struct i2c_client *client   = NULL;
        struct i2c_board_info *board_info= NULL;
        struct asus_bat_config bat_cfg;
        u32 test_flag=0;
        u32 test_major_flag, test_minor_flag;
        struct proc_dir_entry *entry=NULL;

        /* load uboot config */
        ret = fake_load_bat_config(&bat_cfg, &batlow, &test_flag);
        if (ret) {
                BAT_DBG_E("load battery config fail.\n");
                return ret;
        }

        test_major_flag = test_flag & 0x0FFFF;
        test_minor_flag = test_flag >> 16;
        if (test_major_flag & TEST_INFO_NO_REG_POWER)
                test_minor_flag = BIT0;

        if (!bat_cfg.battery_support) {
                BAT_DBG_E("Battery not support. Skip ... \n");
                ret = -ENODEV;
                goto battery_not_support;
        }

        if (bat_cfg.slave_addr != FAKE_I2C_DEFAULT_ADDR && 
                  bat_cfg.slave_addr != BATTERY_I2C_ANY_ADDR) {
                BAT_DBG_E("Not select %s driver. Skip ... \n", 
                                fake_bat_i2c_driver.driver.name );
                ret = -ENODEV;
                goto battery_not_support;
        }

        //turn to jiffeys/s 
        bat_cfg.polling_time *= HZ;
        bat_cfg.critical_polling_time *= HZ;

        //register i2c device
        board_info = &fake_bat_i2c_board_info;
        adapter = i2c_get_adapter(((struct fake_bat_platform_data *)board_info->platform_data)->i2c_bus_id);
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

        mutex_lock(&fake_batt_dev_info_mutex);    
        fake_batt_dev_info.i2c = client;
        fake_batt_dev_info.test_flag = test_minor_flag;

        fake_bat.percentage = 50;
        fake_bat.batt_volt = 3000;
        fake_bat.batt_current = 500;
        fake_bat.batt_temp = 250;
        fake_bat.batt_av_energy = 3000;
        mutex_unlock(&fake_batt_dev_info_mutex);

#if CONFIG_PROC_FS
        entry = create_proc_entry("fake_battery", 0666, NULL);
        if (!entry) {
                BAT_DBG_E("Unable to create proc file\n");
                goto proc_fail;
        }
        entry->read_proc = fake_batt_proc_read;
        entry->write_proc = fake_batt_proc_write;
#endif

        //init battery info & work queue
        ret = asus_battery_init(bat_cfg.polling_time, bat_cfg.critical_polling_time, 1, test_major_flag); //cell type: any number
        if (ret)
                goto asus_battery_init_fail;

        //register i2c driver
        ret =  i2c_add_driver(&fake_bat_i2c_driver);
        if (ret) {
                BAT_DBG("register fake battery i2c driver failed\n");
                goto i2c_register_driver_fail;
        }

        return ret;

i2c_register_driver_fail:
        asus_battery_exit();
asus_battery_init_fail:
#if CONFIG_PROC_FS
proc_fail:
#endif
        i2c_unregister_device(client);
register_i2c_device_fail:
get_adapter_fail:
battery_not_support:
        return ret;
}
late_initcall(fake_bat_i2c_init);

static void __exit fake_bat_i2c_exit(void)
{
        struct i2c_client *client = NULL;
        
        asus_battery_exit();

        mutex_lock(&fake_batt_dev_info_mutex);    
        client = fake_batt_dev_info.i2c;
        mutex_unlock(&fake_batt_dev_info_mutex);

        i2c_unregister_device(fake_batt_dev_info.i2c);
        i2c_del_driver(&fake_bat_i2c_driver);


        BAT_DBG("%s exit\n", __func__);
}
module_exit(fake_bat_i2c_exit);

MODULE_AUTHOR("Wade1_Chen@asus.com");
MODULE_DESCRIPTION("fake driver");
MODULE_LICENSE("GPL");
