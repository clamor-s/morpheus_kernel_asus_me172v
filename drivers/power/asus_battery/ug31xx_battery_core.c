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
#include "ug31xx_battery_core.h"
#include "ug31xx_api.h"
#include "ug31xx_i2c.h"
#include "ug31xx_reg_def.h"

#if CONFIG_PROC_FS
#include "ug31xx_proc_fs.h"
#endif

//extern function 
extern int wmt_getsyspara(char *varname, char *varval, int *varlen);
extern int wmt_setsyspara(char *varname, char *varval);
//
static struct asus_batgpio_set  batlow ;

static struct workqueue_struct *batlow_wq;
static struct work_struct batlow_work;

static struct delayed_work refresh_info_work;

static struct dev_func ug31xx_tbl;

struct battery_dev_info ug31_dev_info;
DEFINE_MUTEX(ug31_dev_info_mutex);

GG_DEVICE_INFO ug31_gauge_dev_info;
GG_CAPACITY    ug31_gauge_dev_capacity;    
DEFINE_MUTEX(ug31_gauge_mutex);

extern char FactoryGGBXFile[];

int ug31xx_asus_battery_dev_read_percentage(void)
{
        int percent=0;
        struct i2c_client *client=NULL;

        mutex_lock(&ug31_dev_info_mutex);    
        client = ug31_dev_info.i2c;
        mutex_unlock(&ug31_dev_info_mutex);

        mutex_lock(&ug31_gauge_mutex);    
        percent = ug31_gauge_dev_capacity.RSOC;
        mutex_unlock(&ug31_gauge_mutex);    

        return percent;
};

int ug31xx_asus_battery_dev_read_volt(void)
{
        int voltage_mV=0;
        struct i2c_client *client=NULL;

        mutex_lock(&ug31_dev_info_mutex);    
        client = ug31_dev_info.i2c;
        mutex_unlock(&ug31_dev_info_mutex);

        mutex_lock(&ug31_gauge_mutex);    
        voltage_mV = ug31_gauge_dev_info.voltage_mV;
        mutex_unlock(&ug31_gauge_mutex);    

        return voltage_mV;
}

int ug31xx_asus_battery_dev_read_current(void)
{
        int current_mA;
        struct i2c_client *client=NULL;

        mutex_lock(&ug31_dev_info_mutex);    
        client = ug31_dev_info.i2c;
        mutex_unlock(&ug31_dev_info_mutex);

        mutex_lock(&ug31_gauge_mutex);    
        current_mA = ug31_gauge_dev_info.AveCurrent_mA;
        mutex_unlock(&ug31_gauge_mutex);    

        return current_mA;
}

int ug31xx_asus_battery_dev_read_av_energy(void)
{
        int av_energy=0;
        struct i2c_client *client=NULL;

        mutex_lock(&ug31_dev_info_mutex);    
        client = ug31_dev_info.i2c;
        mutex_unlock(&ug31_dev_info_mutex);

        mutex_lock(&ug31_gauge_mutex);    
        av_energy = ug31_gauge_dev_capacity.LMD * ug31_gauge_dev_info.voltage_mV / 1000;
        mutex_unlock(&ug31_gauge_mutex);    

        return av_energy;
}

int ug31xx_asus_battery_dev_read_temp(void)
{
        int temperature=0;
        struct i2c_client *client=NULL;

        mutex_lock(&ug31_dev_info_mutex);    
        client = ug31_dev_info.i2c;
        mutex_unlock(&ug31_dev_info_mutex);

        mutex_lock(&ug31_gauge_mutex);    
        temperature = ug31_gauge_dev_info.ET ;
        mutex_unlock(&ug31_gauge_mutex);    
        
        return temperature;
}

static void ug31xx_refresh_info(struct work_struct *dat)
{
        int status=0;
        GG_DEVICE_INFO gauge_dev_info;
        GG_CAPACITY    gauge_dev_capacity;    
        struct i2c_client *client=NULL;

        mutex_lock(&ug31_dev_info_mutex);    
        client = ug31_dev_info.i2c;
        mutex_unlock(&ug31_dev_info_mutex);

        mutex_lock(&ug31_gauge_mutex);    

        status = upiGG_ReadDeviceInfo(&gauge_dev_info);
        if (status !=  UG_READ_DEVICE_INFO_SUCCESS) {
                dev_err(&client->dev, "Read device info fail. %d\n", status);
                goto ug31_read_dev_info_fail;
        }
        upiGG_ReadCapacity(&gauge_dev_capacity);

        //nothing wrong, then update global information
        ug31_gauge_dev_info = gauge_dev_info;
        ug31_gauge_dev_capacity = gauge_dev_capacity;    
                
        mutex_unlock(&ug31_gauge_mutex);

        if (dat) //not local call this function.
                schedule_delayed_work(&refresh_info_work, INFO_UPDATE_TIME);

        return;

ug31_read_dev_info_fail:
        mutex_unlock(&ug31_gauge_mutex);
        if (dat) //not local call this function.
                schedule_delayed_work(&refresh_info_work, INFO_UPDATE_TIME);
}

static int ug31xx_hw_config(struct i2c_client *client)
{
        int ret;

        ret = upiGG_Initial((GGBX_FILE_HEADER *)FactoryGGBXFile);
        if (ret != UG_INIT_SUCCESS) {
                dev_err( &client->dev, "upi battery init fail. %d\n", ret);
                goto upiGG_init_fail;
        }

        //update global gauge variable information
        ug31xx_refresh_info(NULL);

        return 0;

upiGG_init_fail:
        return ret;
}

//int gpio_irq_config(struct i2c_client *client) 
//{
//        int ret=0;
//
//        if (batlow.active < 0)
//                return 0;
//
//        //config gpio
//        write_reg(batlow.ctraddr, 0, batlow.bitmap);
//        write_reg(batlow.icaddr, batlow.bitmap, 0); //config GPI
//        write_reg(batlow.ipcaddr, 0, batlow.bitmap); //pull up/down enable
//	if(batlow.active & 0x1){
//                write_reg(batlow.ipdaddr, 0, batlow.bitmap);
//	}else{
//                write_reg(batlow.ipdaddr, batlow.bitmap, 0);
//	}
//
//        //interrupt not support, skip..
//        if (batlow.int_active < 0)
//                return 0;
//
//
//        //clear status, disable irq.
//        write_reg(batlow.int_sts_addr, batlow.int_bitmap, 0);
//        write_reg(batlow.int_ctrl_addr, batlow.int_bitmap, 0);
//
//        ret = request_irq(IRQ_PMC_WAKEUP, 
//                        batlow_irq_handler, IRQF_SHARED, client->name, client);
//        if (ret) {
//                dev_err(&client->dev, "request irq fail. %d\n", ret);
//                return ret;
//        }
//
//        write_reg(batlow.int_type_addr, 
//                batlow.int_type_val_clr, batlow.int_type_val_set);
//        write_reg(batlow.int_ctrl_addr, 0, batlow.int_bitmap);
//
//        return 0;
//}

static int __devexit ug31xx_bat_i2c_remove(struct i2c_client *i2c)
{
    dev_info(&i2c->dev, "%s\n", __func__);
    return 0;
}

static int __devexit ug31xx_bat_i2c_shutdown(struct i2c_client *i2c)
{
    dev_info(&i2c->dev, "%s\n", __func__);

    cancel_work_sync(&batlow_work);
    cancel_delayed_work_sync(&refresh_info_work);
    asus_battery_exit();
}

#ifdef CONFIG_PM
static int ug31xx_bat_i2c_suspend(struct i2c_client *i2c, pm_message_t message)
{
        int ret;

        dev_info(&i2c->dev, "%s\n", __func__);

        ret = upiGG_PreSuspend();
        if (ret != UG_READ_DEVICE_INFO_SUCCESS) {
                dev_err(&i2c->dev, "[%s] fail in suspend %d\n", __func__, ret);
        }
	asus_battery_suspend();
    return 0;
}

static int ug31xx_bat_i2c_resume(struct i2c_client *i2c)
{
        int ret;

        dev_info(&i2c->dev, "%s\n", __func__);
        ret = upiGG_Wakeup();
        if (ret != UG_READ_DEVICE_INFO_SUCCESS) {
                dev_err(&i2c->dev, "[%s] fail in suspend %d\n", __func__, ret);
        }
	asus_battery_resume();
        return 0;
}

#else
#define ug31xx_bat_i2c_suspend NULL
#define ug31xx_bat_i2c_resume  NULL
#endif
static int __devinit 
ug31xx_bat_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
{
        int ret = 0;

        mutex_lock(&ug31_dev_info_mutex);    
        ug31_dev_info.status = DEV_INIT;
        mutex_unlock(&ug31_dev_info_mutex);

        ret = ug31xx_hw_config(i2c);
        if (ret) {
                dev_err(&i2c->dev, "%s hw config init fail.%d\n", __func__, ret);
                goto hw_config_fail;
        }
        //ug31xx_dump_info(i2c);

        dev_info(&i2c->dev, "%s init done.", __func__);

        ug31xx_tbl.read_percentage = ug31xx_asus_battery_dev_read_percentage;
        ug31xx_tbl.read_current = ug31xx_asus_battery_dev_read_current;
        ug31xx_tbl.read_volt = ug31xx_asus_battery_dev_read_volt;
        ug31xx_tbl.read_av_energy = ug31xx_asus_battery_dev_read_av_energy;
        ug31xx_tbl.read_temp = ug31xx_asus_battery_dev_read_temp;
        ret = asus_register_power_supply(&i2c->dev, &ug31xx_tbl);
        if (ret)
                goto power_register_fail;

        //INIT_WORK(&batlow_work,batlow_work_func);
	INIT_DELAYED_WORK_DEFERRABLE(&refresh_info_work, ug31xx_refresh_info);
        schedule_delayed_work(&refresh_info_work, INFO_UPDATE_TIME);

        mutex_lock(&ug31_dev_info_mutex);    
        ug31_dev_info.status = DEV_INIT_OK;
        mutex_unlock(&ug31_dev_info_mutex);

        //gpio_irq_config(i2c);
        dev_info(&i2c->dev, "register done. %s", __func__);

        return 0;

power_register_fail:
hw_config_fail:
        return ret;
}

static struct ug31xx_bat_platform_data ug31xx_bat_pdata = {
        .i2c_bus_id      = UG31XX_I2C_BUS,
};

static struct i2c_board_info ug31xx_bat_i2c_board_info = {
        .type          = UG31XX_DEV_NAME,
        .flags         = 0x00,
        .addr          = UG31XX_I2C_DEFAULT_ADDR,
        .archdata      = NULL,
        .irq           = -1,
        .platform_data = &ug31xx_bat_pdata,
};


static struct i2c_device_id ug31xx_bat_i2c_id[] = {
	{ UG31XX_DEV_NAME, 0 },
	{ },
};

static struct i2c_driver ug31xx_bat_i2c_driver = {
        .driver    = {
                .name  = UG31XX_DEV_NAME,
                .owner = THIS_MODULE,
        },
        .probe     = ug31xx_bat_i2c_probe,
        .remove    = ug31xx_bat_i2c_remove,
        .suspend   = ug31xx_bat_i2c_suspend,
        .resume    = ug31xx_bat_i2c_resume,
        .shutdown  = ug31xx_bat_i2c_shutdown,
        .id_table  = ug31xx_bat_i2c_id,
};

static char uboot_var_buf[800];
static int uboot_var_buf_size = sizeof(uboot_var_buf); 

int ug31xx_restore_config()
{
	char varname[] = "asus.io.bat";
	char varname_debug[] = "asus.debug.bat";
        char varname_bak[] = "asus.io.bat.bak";
        char varname_debug_bak[] = "asus.debug.bat.bak";
        struct asus_bat_config bat_cfg;
        u32 test_flag=0;
        u32 test_major_flag, test_minor_flag;

        memset(&bat_cfg, 0, sizeof(bat_cfg));
        memset(uboot_var_buf, 0, sizeof(uboot_var_buf));

        //Get backup var first. if ONE_SHOT support, these token MUST be exist.
	if (!wmt_getsyspara(varname_bak, uboot_var_buf, &uboot_var_buf_size) ){
                sscanf(uboot_var_buf,"%d:%d:%d:%x",
                                &bat_cfg.battery_support, 
                                &bat_cfg.polling_time, 
                                &bat_cfg.critical_polling_time,
                                &bat_cfg.slave_addr
                        );
        } else {
                BAT_DBG_E("[ug31xx] Get %s error. Default not support battery\n", varname_bak);
                bat_cfg.battery_support = 0;
                bat_cfg.polling_time = BATTERY_DEFAULT_POLL_TIME/HZ;
                bat_cfg.critical_polling_time = BATTERY_CRITICAL_POLL_TIME/HZ;
                bat_cfg.slave_addr = UG31XX_I2C_DEFAULT_ADDR;
        }

	if (!wmt_getsyspara(varname_debug_bak, uboot_var_buf, &uboot_var_buf_size) ){
                sscanf(uboot_var_buf,"%x", &test_flag);
        } else {
                BAT_DBG_E("[ug31xx] Get %s error. Not use debug\n", varname_debug_bak);
                test_flag = 0;
        }

        //filter one shot command
        test_major_flag = test_flag & 0x0FFFF;
        test_minor_flag = test_flag >> 16;
        test_minor_flag &= ~TEST_UG31_PROC_ONE_SHOT;
        test_flag = (test_minor_flag<<16) | test_major_flag;

        //restore asus.io.bat
        memset(uboot_var_buf, 0, sizeof(uboot_var_buf));
        snprintf(uboot_var_buf, sizeof(uboot_var_buf), "%d:%d:%d:0x%x",
                      bat_cfg.battery_support, 
                      bat_cfg.polling_time, 
                      bat_cfg.critical_polling_time,
                      bat_cfg.slave_addr
                ); 
        BAT_DBG_E("[ug31xx] restore %s = %s\n", varname, uboot_var_buf);

        if (wmt_setsyspara(varname, uboot_var_buf)) {
                BAT_DBG_E("[ug31xx] restore %s fail.\n", varname);
        }
        
        //restore asus.debug.bat
        memset(uboot_var_buf, 0, sizeof(uboot_var_buf));
        snprintf(uboot_var_buf, sizeof(uboot_var_buf), "0x%x", test_flag);
        BAT_DBG_E("[ug31xx] restore %s = %s\n", varname_debug, uboot_var_buf);

        if (wmt_setsyspara(varname_debug, uboot_var_buf)) {
                BAT_DBG_E("[ug31xx] restore %s fail.\n", varname_debug);
        }

        BAT_DBG_E("[ug31xx] restore done.\n");

        return 0;
}

int ug31xx_load_bat_config(
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
                //ug31xx battery is default support.
                p_bat_cfg->battery_support = 1;
                p_bat_cfg->polling_time = BATTERY_DEFAULT_POLL_TIME;
                p_bat_cfg->critical_polling_time = BATTERY_CRITICAL_POLL_TIME;
                p_bat_cfg->slave_addr = UG31XX_I2C_DEFAULT_ADDR;
                
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
                BAT_DBG_E("[ug31xx] warning: asus.gp.bat use default value.");
                p_batlow->active = 0; 
                p_batlow->bitmap = 0x01; //enable battery low gpio pin
                p_batlow->ctraddr = 0xd8110042;
                p_batlow->icaddr = 0xd8110082;
                p_batlow->idaddr = 0xd8110000;
                p_batlow->ipcaddr = 0xd8110482;
                p_batlow->ipdaddr = 0xd81104c0;
                p_batlow->int_active = -1; //interrupt not support 
        }


	if (!wmt_getsyspara(varname_debug, uboot_var_buf, &uboot_var_buf_size) ) {
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

                BAT_DBG_E("[ug31xx] GPIO base not correct.\n");
                p_batlow->active = 0;
        }

        return 0;
}

static int __init ug31xx_bat_i2c_init(void)
{
        int ret = 0;
        struct i2c_adapter *adapter = NULL;
        struct i2c_client *client   = NULL;
        struct i2c_board_info *board_info= NULL;
        struct asus_bat_config bat_cfg;
        
        u32 test_flag=0;
        u32 test_major_flag, test_minor_flag;

        /* load uboot config */
        ret = ug31xx_load_bat_config(&bat_cfg, &batlow, &test_flag);
        if (ret) {
                BAT_DBG_E("load battery config fail.\n");
                goto battery_not_support;
        }

        test_major_flag = test_flag & 0x0FFFF;
        test_minor_flag = test_flag >> 16;
        if (test_major_flag & TEST_INFO_NO_REG_POWER)
                test_minor_flag = TEST_UG31_PROC_INFO_DUMP | 
                                TEST_UG31_PROC_REG_DUMP | 
                                TEST_UG31_PROC_GGB_INFO_DUMP |
                                TEST_UG31_PROC_I2C_PROTOCOL;

        if (test_minor_flag & TEST_UG31_PROC_ONE_SHOT) {
                ret = ug31xx_restore_config();
                if (ret) {
                        goto battery_not_support;
                }
        }


        if (!bat_cfg.battery_support) {
                BAT_DBG_E("Battery not support. Skip ... \n");
                ret = -ENODEV;
                goto battery_not_support;
        }

        if (bat_cfg.slave_addr != UG31XX_I2C_DEFAULT_ADDR && 
                 bat_cfg.slave_addr != BATTERY_I2C_ANY_ADDR) {
                BAT_DBG_E("Not select %s driver. Skip ... \n", 
                                ug31xx_bat_i2c_driver.driver.name );
                ret = -ENODEV;
                goto battery_not_support;
        }

        //turn to jiffeys/s 
        bat_cfg.polling_time *= HZ;
        bat_cfg.critical_polling_time *= HZ;

        //register i2c device
        board_info = &ug31xx_bat_i2c_board_info;
        adapter = i2c_get_adapter(((struct ug31xx_bat_platform_data *)board_info->platform_data)->i2c_bus_id);
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

        mutex_lock(&ug31_dev_info_mutex);    
        ug31_dev_info.i2c = client;
        ug31_dev_info.test_flag = test_minor_flag;
        mutex_unlock(&ug31_dev_info_mutex);

#if CONFIG_PROC_FS
        ret = ug31xx_register_proc_fs();
        if (ret) {
                BAT_DBG_E("Unable to create proc file\n");
                goto proc_fail;
        }
#endif

        //init battery info & work queue
        ret = asus_battery_init(bat_cfg.polling_time, bat_cfg.critical_polling_time, 1, test_major_flag);
        if (ret)
                goto asus_battery_init_fail;

        //register i2c driver
        ret =  i2c_add_driver(&ug31xx_bat_i2c_driver);
        if (ret) {
                BAT_DBG("register ug31xx battery i2c driver failed\n");
                goto i2c_register_driver_fail;
        }

        return ret;

i2c_register_driver_fail:
        asus_battery_exit();
asus_battery_init_fail:
        i2c_unregister_device(client);
#if CONFIG_PROC_FS
proc_fail:
#endif
register_i2c_device_fail:
get_adapter_fail:
battery_not_support:
        return ret;
}
late_initcall(ug31xx_bat_i2c_init);

static void __exit ug31xx_bat_i2c_exit(void)
{
        struct i2c_client *client = NULL;
        
        asus_battery_exit();

        mutex_lock(&ug31_dev_info_mutex);    
        client = ug31_dev_info.i2c;
        mutex_unlock(&ug31_dev_info_mutex);

        i2c_unregister_device(ug31_dev_info.i2c);
        i2c_del_driver(&ug31xx_bat_i2c_driver);


        BAT_DBG("%s exit\n", __func__);
}
module_exit(ug31xx_bat_i2c_exit);

MODULE_AUTHOR("Wade1_Chen@asus.com");
MODULE_DESCRIPTION("battery ug31xx driver");
MODULE_LICENSE("GPL");
