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
#include <linux/thermal.h>
#include <linux/reboot.h>
#include <linux/cpufreq.h>
#include <linux/err.h>


#include "nct7717u.h"
#include "nct7717u_proc_fs.h"

struct thermal_dev_info nct7717u_dev_info;
DEFINE_MUTEX(nct7717u_dev_info_mutex);

#define NCT_TRIP_STAGE0         0
#define NCT_TRIP_STAGE1         1
#define NCT_TRIP_STAGE2         2
#define NCT_TRIP_NUM            3 

//extern function 
extern int wmt_getsyspara(char *varname, char *varval, int *varlen);
//

int nct7717u_read_i2c(struct i2c_client *client, 
                u8 reg, int *rt_value, int b_single)
{
        int ret;

        if (!rt_value)
                return -EINVAL;

	if (!client || !client->adapter)
		return -ENODEV;

        if (b_single)
                ret = i2c_smbus_read_byte_data(client, reg);
        else
                ret = i2c_smbus_read_word_data(client, reg);

        if (ret >= 0) {
                *rt_value = ret;
                ret = 0;
        }

        return ret;
}

int nct7717u_write_i2c(struct i2c_client *client, 
                u8 reg, int value, int b_single)
{
        int ret;

	if (!client || !client->adapter)
		return -ENODEV;

        if (b_single)
                ret = i2c_smbus_write_byte_data(client, reg, value);
        else
                ret = i2c_smbus_write_word_data(client, reg, value);

	return ret;
}


#ifdef CONFIG_PM
static int nct7717u_i2c_suspend(struct i2c_client *i2c, pm_message_t message)
{
        dev_info(&i2c->dev, "%s\n", __func__);
	cancel_delayed_work_sync(&nct7717u_dev_info.tz_dev->poll_queue);
        return 0;
}

static int nct7717u_i2c_resume(struct i2c_client *i2c)
{
        int delay;
	struct thermal_zone_device *tz= NULL;

        dev_info(&i2c->dev, "%s\n", __func__);

        mutex_lock(&nct7717u_dev_info_mutex);    
        tz= nct7717u_dev_info.tz_dev;
        delay = nct7717u_dev_info.tm_cfg.polling_time;
        mutex_unlock(&nct7717u_dev_info_mutex);

	if (delay > 1000)
		schedule_delayed_work(&(tz->poll_queue),
				      round_jiffies(msecs_to_jiffies(delay)));
	else
		schedule_delayed_work(&(tz->poll_queue),
				      msecs_to_jiffies(delay));
        return 0;
}

#else
#define nct7717u_i2c_suspend NULL
#define nct7717u_i2c_resume  NULL
#endif


static int __devexit nct7717u_i2c_shutdown(struct i2c_client *i2c)
{
        dev_info(&i2c->dev, "%s\n", __func__);
	cancel_delayed_work_sync(&nct7717u_dev_info.tz_dev->poll_queue);
        return 0;
}

static int __devexit nct7717u_i2c_remove(struct i2c_client *i2c)
{
        dev_info(&i2c->dev, "%s\n", __func__);

        return 0;
}

static int nct7717u_tz_get_temp(struct thermal_zone_device *thermal,
			      unsigned long *temp)
{
	int ret;
        static struct i2c_client *client=NULL;
        int tmp_buf=0;
        int replace_temp = 0;

	if (!thermal|| !temp)
                return -EINVAL;

        mutex_lock(&nct7717u_dev_info_mutex);    
        replace_temp = nct7717u_dev_info.replace_temp;
        mutex_unlock(&nct7717u_dev_info_mutex);

//        TM_DBG("[%s] Read temp.\n", __func__);
        if (replace_temp) {
                *temp = M_TEMP(replace_temp);
                ret = 0;
                goto out;
        }
        client = nct7717u_dev_info.i2c;

	ret = nct7717u_read_i2c(client, NCT7717U_DIODE_TEMP, &tmp_buf, 1);
	if (ret < 0) {
                TM_DBG_E("Read temp. error\n");
                goto out;
        }
        if (tmp_buf & BIT7) {
            printk("davidwangs nct7717u register %d\n", tmp_buf);
            tmp_buf = 0;
                /*tmp_buf &= (BIT7-1);*/
                /*tmp_buf = -tmp_buf;*/
        }

	*temp = M_TEMP(tmp_buf);
        ret = 0;

out:
	return ret;
}

static int nct7717u_tz_get_mode(struct thermal_zone_device *thermal,
			      enum thermal_device_mode *mode)
{
	if (!thermal|| !mode)
		return -EINVAL;

        mutex_lock(&nct7717u_dev_info_mutex);    

	*mode = nct7717u_dev_info.mode;

        mutex_unlock(&nct7717u_dev_info_mutex);

	return 0;
}

static int nct7717u_tz_set_mode(struct thermal_zone_device *thermal,
			      enum thermal_device_mode mode)
{

	if (!thermal)
		return -EINVAL;

        mutex_lock(&nct7717u_dev_info_mutex);    

	nct7717u_dev_info.mode = mode;

        mutex_unlock(&nct7717u_dev_info_mutex);

	return 0;
}

struct trip_map_entry {
        int stage;
        int temp;
        int type;
};

struct trip_map_entry trip_map[] = {
        {       NCT_TRIP_STAGE2, M_TEMP(85),  THERMAL_TRIP_CRITICAL     },
        {       NCT_TRIP_STAGE1, M_TEMP(70),  THERMAL_TRIP_HOT          },
        {       NCT_TRIP_STAGE0, M_TEMP(65),  THERMAL_TRIP_CONFIGURABLE_LOW}, 
};

static int nct7717u_tz_get_trip_type(struct thermal_zone_device *thermal,
				   int trip, enum thermal_trip_type *type)
{
        int i;

	if (!thermal || trip < 0 || !type)
		return -EINVAL;

        for (i=0; i<NCT_TRIP_NUM; i++) {
                if (trip == trip_map[i].stage) {
                        *type = trip_map[i].type;
                        return 0;
                }
        }

        TM_DBG_E("[%s] Not found map stage, trip = %d\n", __func__, trip);

        return -EINVAL;
}

static int nct7717u_tz_get_trip_temp(struct thermal_zone_device *thermal,
				   int trip, unsigned long *temp)
{
        int i;

	if (!thermal || trip < 0 || !temp)
                return -EINVAL;

        for (i=0; i < NCT_TRIP_NUM; i++) {
                if (trip == trip_map[i].stage) {
                        *temp = trip_map[i].temp;
                        return 0;
                }
        }

        TM_DBG_E("[%s] Not found map temp, trip = %d\n", __func__, trip);

        return -EINVAL;

}

/*
static int nct7717u_bind(struct thermal_zone_device *thermal,
                struct thermal_cooling_device *cdev)
{

        TM_DBG("[%s] type %s\n", __func__, cdev->type);

        if (strcmp(cdev->type, "Processor")) 
                return 0; // not for us 

        if (thermal_zone_bind_cooling_device(thermal, NCT_TRIP_STAGE0, cdev)) {
                pr_err("error binding cooling dev\n");
                return -EINVAL;
        }
        return 0;
}
static int nct7717u_unbind(struct thermal_zone_device *thermal,
                struct thermal_cooling_device *cdev)
{
        TM_DBG("[%s] type %s\n", __func__, cdev->type);

        if (strcmp(cdev->type, "Processor")) 
                return 0; // not for us 

        if (thermal_zone_unbind_cooling_device(thermal, NCT_TRIP_STAGE0, cdev)) {
                pr_err("error unbinding cooling dev\n");
                return -EINVAL;
        }
        return 0;
}
*/

static void do_poweroff(struct work_struct *work)
{
	kernel_power_off();
}

static DECLARE_WORK(poweroff_work, do_poweroff);

static unsigned int recorded_max_freq = 0;

static int nct7717u_notify(struct thermal_zone_device *tz, int trip, enum thermal_trip_type type)
{
        int mode=0;
	struct thermal_cooling_device_instance *instance = NULL;
	struct cpufreq_policy *policy = NULL;
        int ret;
        int need_update=0;
        u32 cpu;

        //TM_DBG("[%s] trip %d, type %d\n", __func__, trip, type);
        //

        if (type == THERMAL_TRIP_CRITICAL) {
                schedule_work(&poweroff_work);
                return 0;
        }

	for_each_online_cpu(cpu) {
                need_update = 0;

                policy = cpufreq_cpu_get(cpu);
                if (!policy) {
                        TM_DBG_E("Can't get cpu policy %d\n", cpu);
                        continue;
                }

                if (type == THERMAL_TRIP_CONFIGURABLE_LOW) { // scaling all states
                        // if (policy->max != policy->cpuinfo.max_freq) {
			if (recorded_max_freq!=0) {
                                policy->user_policy.governor = policy->governor;
                                policy->user_policy.governor = policy->governor;
                                policy->user_policy.policy = policy->policy;
                                policy->user_policy.min = policy->min;
                                policy->user_policy.max = (recorded_max_freq>policy->max)? recorded_max_freq : policy->max;
				recorded_max_freq = 0;
                                need_update = 1;
                        }

                } else if (type == THERMAL_TRIP_HOT) { //scaling only min freq
                        if (policy->max != policy->cpuinfo.min_freq) {				
                                policy->user_policy.governor = policy->governor;
                                policy->user_policy.governor = policy->governor;
                                policy->user_policy.policy = policy->policy;
                                policy->user_policy.min = policy->min;
                                policy->user_policy.max = policy->cpuinfo.min_freq;
				if(recorded_max_freq == 0)
					recorded_max_freq = policy->max;
                                need_update = 1;
                        }
                } 

                ret = 0;
                if (need_update) {
                        TM_DBG("[%s] update cpu policy\n", __func__);
                        ret = cpufreq_update_policy(cpu);
                }
                cpufreq_cpu_put(policy);
                if (ret) {
                        TM_DBG_E("[%s] Fail to set policy at cpu %d\n", __func__, cpu);
                }
	}
        

        /* other cases about TRIP_HOT */

        return 0;
}

static struct nct7717u_platform_data nct7717u_pdata = {
        .i2c_bus_id      = NCT7717U_I2C_BUS,
};

static struct i2c_board_info nct7717u_i2c_board_info = {
        .type          = NCT7717U_DEV_NAME,
        .flags         = 0x00,
        .addr          = NCT7717U_DEFAULT_ADDR,
        .archdata      = NULL,
        .irq           = -1,
        .platform_data = &nct7717u_pdata,
};

static struct i2c_device_id nct7717u_i2c_id[] = {
	{ NCT7717U_DEV_NAME, 0 },
	{ },
};

static int __devinit nct7717u_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id);
static struct i2c_driver nct7717u_i2c_driver = {
        .driver    = {
                .name  = NCT7717U_DEV_NAME,
                .owner = THIS_MODULE,
        },
        .probe     = nct7717u_i2c_probe,
        .remove    = nct7717u_i2c_remove,
        .shutdown  = nct7717u_i2c_shutdown,
        .suspend   = nct7717u_i2c_suspend,
        .resume    = nct7717u_i2c_resume,
        .id_table  = nct7717u_i2c_id,
};

struct info_entry {
        char name[30];
        u16 cmd;
};

static struct info_entry dump_info_tbl[] = {
        {"Diod Temp.", NCT7717U_DIODE_TEMP},
        {"Alert Status", NCT7717U_ALERT_STATUS},
        {"Alert Config", NCT7717U_CONFIG},
        {"Conversion Rate", NCT7717U_CONV_RATE},
        {"Alert Temp.", NCT7717U_ALERT_TEMP},
        {"Alert Mode", NCT7717U_ALERT_MODE},
        {"Chip ID", NCT7717U_CHIP_ID},
        {"Vendor ID", NCT7717U_VENDOR_ID},
        {"Dev ID", NCT7717U_DEV_ID},
};

static void nct7717u_dump_info(struct i2c_client *client)
{
        u32 i;

        dev_info(&client->dev, "[%s]enter\n", __func__);

        for (i=0; i<sizeof(dump_info_tbl)/sizeof(struct info_entry); i++){
                int tmp_buf=0, ret=0;

                ret = nct7717u_read_i2c(client, dump_info_tbl[i].cmd, &tmp_buf, 1);
                if (ret < 0) continue;

                dev_info(&client->dev, "%s, 0x%04X\n", dump_info_tbl[i].name, tmp_buf);
        }
}

static struct thermal_zone_device_ops nct7717u_thermal_zone_ops = {
	.get_temp = nct7717u_tz_get_temp,
	.get_mode = nct7717u_tz_get_mode,
	.set_mode = nct7717u_tz_set_mode,
	.get_trip_type = nct7717u_tz_get_trip_type,
	.get_trip_temp = nct7717u_tz_get_trip_temp,
        .notify = nct7717u_notify,
//        .bind = nct7717u_bind,
//        .unbind = nct7717u_unbind,
};

static int __devinit 
nct7717u_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
{
        int ret = 0;
	struct i2c_adapter *adapter = i2c->adapter;
        int polling_time=0;

        dev_info(&i2c->dev, "%s\n", __func__);

        mutex_lock(&nct7717u_dev_info_mutex);    
        nct7717u_dev_info.status = DEV_INIT;
        polling_time = nct7717u_dev_info.tm_cfg.polling_time;
        mutex_unlock(&nct7717u_dev_info_mutex);

        nct7717u_dump_info(i2c);

        nct7717u_dev_info.mode = THERMAL_DEVICE_DISABLED; //driver control, not user space ap.

	nct7717u_dev_info.tz_dev = thermal_zone_device_register(
                                "nct7717u_tz",
				 NCT_TRIP_NUM, 
                                 NULL,
				 &nct7717u_thermal_zone_ops,
				 0, 0, 0, polling_time
                        );
        if (IS_ERR_OR_NULL(nct7717u_dev_info.tz_dev)) {
                dev_err(&i2c->dev, "%s Fail to register thermal zone device\n", __func__);
                goto out;
        }

        mutex_lock(&nct7717u_dev_info_mutex);    
        nct7717u_dev_info.status = DEV_INIT_OK;
        mutex_unlock(&nct7717u_dev_info_mutex);

out:
        return ret;
}


static char uboot_var_buf[800];
static int uboot_var_buf_size = sizeof(uboot_var_buf); 
int nct7717u_load_thermal_config(struct asus_thermal_config *p_thermal_cfg) 
{
	char varname[] = "asus.io.thermal";

        if (!p_thermal_cfg) {
                return -EINVAL;
        }

	if (!wmt_getsyspara(varname, uboot_var_buf, &uboot_var_buf_size) ){
                sscanf(uboot_var_buf,"%d:%d:%x",
                                &p_thermal_cfg->thermal_support, 
                                &p_thermal_cfg->polling_time, 
                                &p_thermal_cfg->slave_addr
                        );
        } else {
                //Oops!! Read uboot parameter fail. Use default instead
                p_thermal_cfg->thermal_support = 1;
                p_thermal_cfg->polling_time = THERMAL_DEFAULT_POLL_TIME;
                p_thermal_cfg->slave_addr = NCT7717U_DEFAULT_ADDR;
        }

        return 0;
}

static int __init nct7717u_i2c_init(void)
{
        int ret = 0;
        struct i2c_adapter *adapter = NULL;
        struct i2c_client *client   = NULL;
        struct i2c_board_info *board_info= NULL;
        struct asus_thermal_config tm_cfg;

        TM_DBG("[%s] enter\n", __func__);

        /* load uboot config */
        ret = nct7717u_load_thermal_config(&tm_cfg);
        if (ret) {
                TM_DBG_E("load thermal config fail.\n");
                goto load_config_fail;
        }

        if (!tm_cfg.thermal_support) {
                TM_DBG_E("Thermal not support. Skip ... \n");
                ret = -ENODEV;
                goto thermal_not_support;
        }

        if (tm_cfg.slave_addr != NCT7717U_DEFAULT_ADDR) {
                TM_DBG_E("Not select %s driver. Skip ... \n", 
                                nct7717u_i2c_driver.driver.name );
                ret = -ENODEV;
                goto thermal_not_support;
        }

        //turn to jiffeys/s 
        if (tm_cfg.polling_time < 3) {
                TM_DBG_E("[%s] Polling time not correct. Set to be 3s\n", __func__);
                tm_cfg.polling_time = 3;
        }
        tm_cfg.polling_time = M_TEMP(tm_cfg.polling_time);

        TM_DBG("[%s] tm_cfg.polling_time %d\n", __func__, tm_cfg.polling_time);

#if CONFIG_PROC_FS
        ret = nct7717u_register_proc_fs();
        if (ret) {
                TM_DBG_E("Unable to create proc file\n");
                goto proc_fail;
        }
#endif

        //register i2c device
        board_info = &nct7717u_i2c_board_info;
        adapter = i2c_get_adapter(((struct nct7717u_platform_data *)board_info->platform_data)->i2c_bus_id);
        if (adapter == NULL) {
                TM_DBG_E("can not get i2c adapter, client address error\n");
                ret = -ENODEV;
                goto get_adapter_fail;
        }

        client = i2c_new_device(adapter, board_info);
        if (client == NULL) {
                TM_DBG_E("allocate i2c client failed\n");
                ret = -ENOMEM;
                goto register_i2c_device_fail;
        }
        i2c_put_adapter(adapter);

        mutex_lock(&nct7717u_dev_info_mutex);    
        nct7717u_dev_info.tm_cfg = tm_cfg;
        nct7717u_dev_info.i2c = client;
        mutex_unlock(&nct7717u_dev_info_mutex);

        //register i2c driver
        ret =  i2c_add_driver(&nct7717u_i2c_driver);
        if (ret) {
                TM_DBG("register nct7717u battery i2c driver failed\n");
                goto i2c_register_driver_fail;
        }


        return ret;

i2c_register_driver_fail:
        i2c_unregister_device(client);
register_i2c_device_fail:
get_adapter_fail:
#if CONFIG_PROC_FS
proc_fail:
#endif
thermal_not_support:
load_config_fail:
        return ret;
}
late_initcall(nct7717u_i2c_init);

static void __exit nct7717u_i2c_exit(void)
{
        struct i2c_client *client = NULL;
        
        TM_DBG("%s exit\n", __func__);
}
module_exit(nct7717u_i2c_exit);

MODULE_AUTHOR("Wade1_Chen@asus.com");
MODULE_DESCRIPTION("thermal nct7717u driver");
MODULE_LICENSE("GPL");
