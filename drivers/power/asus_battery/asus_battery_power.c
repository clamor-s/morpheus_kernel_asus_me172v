/*
 * Copyright (c) 2012, ASUSTek, Inc. All Rights Reserved.
 * Written by wade chen Wade1_Chen@asus.com
 */
#include <linux/module.h>
#include <linux/param.h>
#include <generated/compile.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/slab.h>
#include <asm/unaligned.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#include "asus_battery.h"
#include "asus_battery_proc_fs.h"

#define MAX_DONT_CARE    1000000
#define MIN_DONT_CARE    -999999

//struct delayed_work battery_low_init_work;		//battery low init
struct delayed_work battery_poll_data_work;		//polling data
struct delayed_work detect_cable_work;			//check cable status
//struct delayed_work battery_eng_protect_work;	
struct workqueue_struct *battery_work_queue=NULL;


DEFINE_MUTEX(batt_info_mutex);

static char *supply_list[] = {
        "battery",
};

struct battery_info_reply batt_info = {
        .drv_status = DRV_NOT_READY,
};
//used by usb_to_battery_callback before module init. Still use batt_info_mutex for protection.
u32 pending_cable_status = NO_CABLE; 

static enum power_supply_property asus_battery_props[] = {
        POWER_SUPPLY_PROP_STATUS,				//0x00
        POWER_SUPPLY_PROP_HEALTH,				//0x02
        POWER_SUPPLY_PROP_PRESENT,				//0x03
        POWER_SUPPLY_PROP_TECHNOLOGY,			//0x05
        POWER_SUPPLY_PROP_VOLTAGE_NOW,			//0x0A
        POWER_SUPPLY_PROP_CURRENT_NOW,			//0x0C
        POWER_SUPPLY_PROP_ENERGY_NOW,			//0x1B
        POWER_SUPPLY_PROP_CAPACITY,				//0x1D
        POWER_SUPPLY_PROP_CAPACITY_LEVEL,		//0x1E
        POWER_SUPPLY_PROP_TEMP,					//0x1F
        //	POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW,	//0x21
        //	POWER_SUPPLY_PROP_TIME_TO_EMPTY_AVG,	//0x22
        //	POWER_SUPPLY_PROP_TIME_TO_FULL_NOW,		//0x23
};

static enum power_supply_property asus_power_properties[] = {
        POWER_SUPPLY_PROP_ONLINE,	//0x04
};


static int asus_battery_get_property(struct power_supply *psy, 
                enum power_supply_property psp,
                union power_supply_propval *val);

static int asus_power_get_property(struct power_supply *psy, 
                enum power_supply_property psp,
                union power_supply_propval *val);

static struct power_supply asus_power_supplies[] = {
        {
                .name = "battery",
                .type = POWER_SUPPLY_TYPE_BATTERY,
                .properties = asus_battery_props,
                .num_properties = ARRAY_SIZE(asus_battery_props),
                .get_property = asus_battery_get_property,
        },
        {
                .name = "usb",
                .type = POWER_SUPPLY_TYPE_USB,
                .supplied_to = supply_list,
                .num_supplicants = ARRAY_SIZE(supply_list),
                .properties = asus_power_properties,
                .num_properties = ARRAY_SIZE(asus_power_properties),
                .get_property = asus_power_get_property,
        },
        {
                .name = "ac",
                .type = POWER_SUPPLY_TYPE_MAINS,
                .supplied_to = supply_list,
                .num_supplicants = ARRAY_SIZE(supply_list),
                .properties = asus_power_properties,
                .num_properties = ARRAY_SIZE(asus_power_properties),
                .get_property = asus_power_get_property,
        },
};


//
/*
 * Return the battery temperature in tenths of degree Celsius
 * Or < 0 if something fails.
 *
 * NOTICE: it's CRITICAL SECTION.
 */
static int asus_battery_update_temp(void)
{
        int temperature = -EINVAL;
        struct battery_info_reply tmp_batt_info;

        tmp_batt_info = batt_info;

        if (tmp_batt_info.tbl && tmp_batt_info.tbl->read_temp) {
                temperature = tmp_batt_info.tbl->read_temp();
        }

        return temperature;	
}

/*
 * Return the battery Voltage in milivolts
 * Or < 0 if something fails.
 *
 * NOTICE: it's CRITICAL SECTION.
 */

static int asus_battery_update_voltage(void)
{
        int volt = -EINVAL;
        struct battery_info_reply tmp_batt_info;

        tmp_batt_info = batt_info;

        if (tmp_batt_info.tbl && tmp_batt_info.tbl->read_volt) {
                volt = tmp_batt_info.tbl->read_volt();
        }


        return volt;
}

/*
 * Return the battery average current
 * Note that current can be negative signed as well(-65536 ~ 65535).
 * So, the value get from device need add a base(0x10000) to be a positive number if no error. 
 * Or < 0 if something fails.
 * 
 * NOTICE: it's CRITICAL SECTION.
 */
static int asus_battery_update_current(void)
{
        int curr = -EINVAL;
        struct battery_info_reply tmp_batt_info;

        tmp_batt_info = batt_info;

        if (tmp_batt_info.tbl && tmp_batt_info.tbl->read_current) {
                curr = tmp_batt_info.tbl->read_current();
        }

        return curr;
}

/*
 * critical section
 */
static int asus_battery_update_available_energy(void)
{
        int mWhr = -EINVAL;
        struct battery_info_reply tmp_batt_info;

        tmp_batt_info = batt_info;

        if (tmp_batt_info.tbl && tmp_batt_info.tbl->read_av_energy) {
                mWhr = tmp_batt_info.tbl->read_av_energy();
        }

        return mWhr;
}

/*
 * critical section
 */
struct battery_map_capacity {
        int min;
        int max; 
        int delta;
};

// min <= value < max
#define SET_TO_ZERO 1
struct battery_map_capacity batt_cap_map[] = {
        {        50,           101,     0 },
        {        30,            50,    -1 },
        {        15,            30,    -2 },
        {         3,            15,    -3 },
        {         0,             3,    SET_TO_ZERO },
};

static u32 map_battery_percentage(int percentage)
{
        int i;
        int delt=0;

        for (i=0; i<sizeof(batt_cap_map)/sizeof(struct battery_map_capacity); i++) {
                if ( batt_cap_map[i].min <= percentage && percentage < batt_cap_map[i].max) {
                        delt = batt_cap_map[i].delta;
                        if (delt == SET_TO_ZERO) {
                                return 0;
                        } else {
                                return percentage + delt;
                        }
                }
        }

        BAT_DBG_E("[%s] Not map in range %d%%\n", __func__, percentage);

        return percentage;

}

static int asus_battery_update_percentage(void)
{
        u32 percentage=-EINVAL;
        drv_status_t drv_status;
        struct battery_info_reply tmp_batt_info;

        tmp_batt_info = batt_info;
        drv_status = batt_info.drv_status;

        if (tmp_batt_info.tbl && tmp_batt_info.tbl->read_percentage) {
                int adjust_percent = 0, percent = 0;

                percent = tmp_batt_info.tbl->read_percentage();
                adjust_percent = map_battery_percentage(percent);
                percentage = adjust_percent;
                if (percentage == 0) {
                        drv_status = DRV_SHUTDOWN;

                } else if (drv_status == DRV_BATTERY_LOW) { 
                        BAT_DBG_E("(%s) battery low event update.\n", __func__);
                        percentage = 0;
                }
                batt_info.drv_status = drv_status;
        }

        return percentage; //% return adjust percentage, and error code ( < 0)
}


//range:   range_min <= X < range_max
struct lvl_entry {
        char name[30];
        int range_min;
        int range_max;
        int ret_val;
};

struct lvl_entry lvl_tbl[] = {
        { "FULL",      100,    MAX_DONT_CARE,   POWER_SUPPLY_CAPACITY_LEVEL_FULL},  
        { "HIGH",       80,              100,   POWER_SUPPLY_CAPACITY_LEVEL_HIGH},  
        { "NORMAL",     20,               80,   POWER_SUPPLY_CAPACITY_LEVEL_NORMAL},  
        { "LOW",         5,               20,   POWER_SUPPLY_CAPACITY_LEVEL_LOW},  
        { "CRITICAL",    0,                5,   POWER_SUPPLY_CAPACITY_LEVEL_CRITICAL},  
        { "UNKNOWN", MIN_DONT_CARE,  MAX_DONT_CARE,  POWER_SUPPLY_CAPACITY_LEVEL_UNKNOWN},  
};

static int asus_battery_update_capacity_level(int percentage)
{	
        int i=0;

        for (i=0; i<sizeof(lvl_tbl)/sizeof(struct lvl_entry); i++) {
                if (lvl_tbl[i].range_max != MAX_DONT_CARE && 
                        percentage >= lvl_tbl[i].range_max)
                        continue;
                if (lvl_tbl[i].range_min != MIN_DONT_CARE && 
                        lvl_tbl[i].range_min > percentage)
                        continue;
                //range match
                //BAT_DBG("Level: %s\n", lvl_tbl[i].name);
                return lvl_tbl[i].ret_val;
        }

        return -EINVAL;
}

/*
 * critical section
 */
static int asus_battery_update_status(int percentage)
{
        int status;
        struct battery_info_reply tmp_batt_info;
        u32 cable_status;

        tmp_batt_info = batt_info;

        cable_status = tmp_batt_info.cable_status;

        if (percentage == 100) {
                status = POWER_SUPPLY_STATUS_FULL;
        } else if (cable_status == USB_ADAPTER || cable_status == USB_PC) {
                status = POWER_SUPPLY_STATUS_CHARGING;
        } else {
                status = POWER_SUPPLY_STATUS_DISCHARGING;
        }
        //BAT_DBG("battery status = 0x%x \n",status);

        return status;
}

static void asus_battery_get_info(void)
{
        struct battery_info_reply tmp_batt_info;
        int tmp=0;

        mutex_lock(&batt_info_mutex);
        tmp_batt_info = batt_info;

        tmp = asus_battery_update_percentage();
        if (tmp >= 0) tmp_batt_info.percentage = tmp;

        tmp = asus_battery_update_capacity_level(tmp_batt_info.percentage);
        if (tmp >= 0) tmp_batt_info.level = tmp;

        tmp = asus_battery_update_status(tmp_batt_info.percentage);
        if (tmp >= 0) tmp_batt_info.status = tmp;

        tmp = asus_battery_update_voltage();
        if (tmp >= 0) tmp_batt_info.batt_volt = tmp;

        tmp = asus_battery_update_current();
        if (tmp >= 0) tmp_batt_info.batt_current = tmp - 0x10000;

        tmp = asus_battery_update_available_energy();
        if (tmp >= 0) tmp_batt_info.batt_energy = tmp;

        tmp = asus_battery_update_temp();
        if (tmp >= 0) tmp_batt_info.batt_temp = tmp;

        batt_info = tmp_batt_info;
        mutex_unlock(&batt_info_mutex);
}

int asus_battery_low_event()
{
        drv_status_t drv_status;

        drv_status = DRV_BATTERY_LOW;
        batt_info.drv_status = drv_status;

        asus_battery_get_info();
        power_supply_changed(&asus_power_supplies[CHARGER_BATTERY]);
        power_supply_changed(&asus_power_supplies[CHARGER_USB]);
        power_supply_changed(&asus_power_supplies[CHARGER_MAINS]);

        return 0;
}

static void asus_polling_data(struct work_struct *dat)
{
        u32 polling_time;
        u32 critical_polling_time;
        struct battery_info_reply tmp_batt_info;
        int ret=0;

        asus_battery_get_info();
        power_supply_changed(&asus_power_supplies[CHARGER_BATTERY]);
        msleep(10);

        mutex_lock(&batt_info_mutex);
        tmp_batt_info = batt_info;
        mutex_unlock(&batt_info_mutex);

#if defined(CONFIG_ASUS_ENGINEER_MODE) && CONFIG_ASUS_ENGINEER_MODE
        //enable / disable charging 
        if (!(tmp_batt_info.test_flag & TEST_INFO_DISABLE_PROTECT)) {
                if (tmp_batt_info.cable_status == USB_PC || tmp_batt_info.cable_status == USB_ADAPTER) {
                        if (tmp_batt_info.percentage >= 60 ) {
                                ret = smb_charging_op(0);
                        } else if (tmp_batt_info.percentage <= 40) {
                                ret = smb_charging_op(1);
                        }
                        /* 
                         * NOTICE: 40% ~ 60% is free section, 
                         *  that means maybe charging or discharging
                         */
                        if (ret) {
                                BAT_DBG_E("%s fail, ret=%d\n", tmp_batt_info.percentage >= 60 ? 
                                                "disable charging":"enable charging",
                                                ret
                                         );
                        }
                }
        }
#endif

        polling_time = tmp_batt_info.polling_time;
        critical_polling_time = tmp_batt_info.critical_polling_time;

        if(batt_info.percentage > 10 && polling_time > (5*HZ)) {
                queue_delayed_work(battery_work_queue,&battery_poll_data_work, polling_time);

        } else if(batt_info.percentage <= 10 && critical_polling_time > (3*HZ)) {
                //if battery status is critical, polling time shorten.
                queue_delayed_work(battery_work_queue, &battery_poll_data_work, critical_polling_time);
                BAT_DBG("Critical condition!!\n");
        }
}

void update_battery_driver_state(void)
{
        asus_battery_get_info();
        // if the power source changes, all power supplies may change state 
        power_supply_changed(&asus_power_supplies[CHARGER_BATTERY]);
        power_supply_changed(&asus_power_supplies[CHARGER_USB]);
        power_supply_changed(&asus_power_supplies[CHARGER_MAINS]);
}
EXPORT_SYMBOL(update_battery_driver_state);

int get_battery_capacity(void)
{
        int cap=0;

        mutex_lock(&batt_info_mutex);
        cap = batt_info.percentage;
        mutex_unlock(&batt_info_mutex);

        return cap;
        
}
EXPORT_SYMBOL(get_battery_capacity);

static void USB_cable_status_worker(struct work_struct *dat)
{
        update_battery_driver_state();
}

void usb_to_battery_callback(u32 usb_cable_state)
{
        drv_status_t drv_sts;    

        BAT_DBG_E("%s %d\n", __func__, usb_cable_state) ;

        mutex_lock(&batt_info_mutex);
        drv_sts = batt_info.drv_status;
        mutex_unlock(&batt_info_mutex);

        if (drv_sts != DRV_REGISTER_OK) {
                BAT_DBG_E("Battery module not ready\n");

                mutex_lock(&batt_info_mutex);
                pending_cable_status = usb_cable_state;
                mutex_unlock(&batt_info_mutex);
                
                return;
        }

        mutex_lock(&batt_info_mutex);
        batt_info.cable_status = usb_cable_state;
        mutex_unlock(&batt_info_mutex);

        queue_delayed_work(battery_work_queue, &detect_cable_work, 0);
}
EXPORT_SYMBOL(usb_to_battery_callback);

u32 receive_USBcable_type(void)
{
        u32 cable_status;
        struct battery_info_reply tmp_batt_info;

        mutex_lock(&batt_info_mutex);
        tmp_batt_info = batt_info;
        mutex_unlock(&batt_info_mutex);

        cable_status = tmp_batt_info.cable_status ;

        return cable_status;
}
EXPORT_SYMBOL(receive_USBcable_type);

static int asus_battery_get_property(struct power_supply *psy,
                enum power_supply_property psp,
                union power_supply_propval *val)
{
        int ret = 0;
        struct battery_info_reply tmp_batt_info;

        //BAT_DBG("%s 0x%04X\n", __func__, psp);

        mutex_lock(&batt_info_mutex);
        tmp_batt_info = batt_info;
        mutex_unlock(&batt_info_mutex);

        switch (psp) {
        case POWER_SUPPLY_PROP_STATUS:
                val->intval = tmp_batt_info.status;
                break;
        case POWER_SUPPLY_PROP_HEALTH:
                val->intval = POWER_SUPPLY_HEALTH_GOOD;
                break;
        case POWER_SUPPLY_PROP_PRESENT:
                val->intval = tmp_batt_info.present;
                break;
        case POWER_SUPPLY_PROP_TECHNOLOGY:
                val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
                break;
        case POWER_SUPPLY_PROP_VOLTAGE_NOW:
                val->intval = tmp_batt_info.batt_volt * 1000; //android unit is micro-volt
                break;
        case POWER_SUPPLY_PROP_CURRENT_NOW:
                val->intval = tmp_batt_info.batt_current;
                break;
        case POWER_SUPPLY_PROP_ENERGY_NOW:
                val->intval = tmp_batt_info.batt_energy;
                break;
        case POWER_SUPPLY_PROP_CAPACITY:
                val->intval = tmp_batt_info.percentage;
                break;
        case POWER_SUPPLY_PROP_CAPACITY_LEVEL:
                val->intval = tmp_batt_info.level;
                break;
        case POWER_SUPPLY_PROP_TEMP:
                val->intval = tmp_batt_info.batt_temp;
                break;
        default:
                return -EINVAL;
        }

        return ret;
}

static int asus_power_get_property(struct power_supply *psy, 
                enum power_supply_property psp,
                union power_supply_propval *val)
{
        int ret;
        struct battery_info_reply tmp_batt_info;
        u32 cable_status;
        u32 drv_status;

        mutex_lock(&batt_info_mutex);
        tmp_batt_info = batt_info;
        mutex_unlock(&batt_info_mutex);

        cable_status = tmp_batt_info.cable_status;
        drv_status = tmp_batt_info.drv_status;

        ret = 0;
        switch (psp) {
        case POWER_SUPPLY_PROP_ONLINE:
                if (drv_status == DRV_SHUTDOWN) {
                        val->intval = 0;

                }else if (drv_status == DRV_BATTERY_LOW) {
                        val->intval = 0;

                } else if (psy->type == POWER_SUPPLY_TYPE_MAINS) {
                    	val->intval = (cable_status == USB_ADAPTER ? 1 : 0);

                } else if (psy->type == POWER_SUPPLY_TYPE_USB) {
                        val->intval = (cable_status == USB_PC ? 1 : 0);
                } else {
                        ret = -EINVAL;
                }
                break;
        default:
                ret = -EINVAL;
        }

        return ret;
}


int asus_register_power_supply(struct device *dev, struct dev_func *tbl)
{
        int ret;
        int test_flag=0;
        drv_status_t drv_status;

        BAT_DBG_E("%s\n", __func__);

        mutex_lock(&batt_info_mutex);
        drv_status = batt_info.drv_status;
        test_flag = batt_info.test_flag;
        mutex_unlock(&batt_info_mutex);

        if (!dev) {
                BAT_DBG_E("%s, device pointer is NULL.\n", __func__);
                return -EINVAL;
        } else if (!tbl) {
                BAT_DBG_E("%s, dev_func pointer is NULL.\n", __func__);
                return -EINVAL;
        }else if (drv_status != DRV_INIT_OK) {
                BAT_DBG_E("%s, asus_battery not init ok.\n", __func__);
                return -EINVAL;
        } else if (test_flag & TEST_INFO_NO_REG_POWER) {
                BAT_DBG_E("%s, Not register power class.\n", __func__);
                return 0;
        }

        mutex_lock(&batt_info_mutex);
        batt_info.drv_status = DRV_REGISTER;
        batt_info.tbl = tbl;
        mutex_unlock(&batt_info_mutex);

        //register to power supply driver
        ret = power_supply_register(dev, &asus_power_supplies[CHARGER_BATTERY]);
        if (ret) { BAT_DBG_E("Fail to register battery\n"); goto batt_err_reg_fail_battery; }

        ret = power_supply_register(dev, &asus_power_supplies[CHARGER_USB]);
        if (ret) { BAT_DBG_E("Fail to register USB\n"); goto batt_err_reg_fail_usb; }

        ret = power_supply_register(dev, &asus_power_supplies[CHARGER_MAINS]);
        if (ret) { BAT_DBG_E("Fail to register MAINS\n"); goto batt_err_reg_fail_mains; }

        //first update current information
        asus_battery_get_info();

        //all OK.
        mutex_lock(&batt_info_mutex);
        batt_info.drv_status = DRV_REGISTER_OK;
        mutex_unlock(&batt_info_mutex);

        //start working 
        queue_delayed_work(battery_work_queue, &battery_poll_data_work, 
                        BATTERY_DEFAULT_POLL_TIME);
//        queue_delayed_work(battery_work_queue, &battery_eng_protect_work, 
//                        BATTERY_PROTECT_DEFAULT_POLL_TIME);

        BAT_DBG("%s register OK.\n", __func__);
        return 0;

batt_err_reg_fail_mains:
        power_supply_unregister(&asus_power_supplies[CHARGER_USB]);
batt_err_reg_fail_usb:
        power_supply_unregister(&asus_power_supplies[CHARGER_BATTERY]);
batt_err_reg_fail_battery:

        return ret;
}
EXPORT_SYMBOL(asus_register_power_supply);


void asus_battery_resume(void){
        queue_delayed_work(battery_work_queue, &battery_poll_data_work, 0);
}


void asus_battery_suspend(void)
{
	cancel_delayed_work_sync(&battery_poll_data_work);
	cancel_delayed_work_sync(&detect_cable_work);
}


int asus_battery_init(
        u32 polling_time, 
        u32 critical_polling_time, 
        u32 cell_status,
        u32 test_flag
)
{
        int ret=0;
        drv_status_t drv_sts;    

        BAT_DBG("%s, %d, %d, 0x%08X\n", __func__, polling_time, critical_polling_time, test_flag);

        mutex_lock(&batt_info_mutex);
        drv_sts = batt_info.drv_status;
        mutex_unlock(&batt_info_mutex);

        if (drv_sts != DRV_NOT_READY) {
                //other battery device registered.
                BAT_DBG_E("Error!! Already registered by other driver\n");
                ret = -EINVAL;
                if (test_flag & TEST_INFO_NO_REG_POWER) {
                        ret = 0;
                }
                                
                goto already_init;

        }

        mutex_lock(&batt_info_mutex);
        batt_info.drv_status = DRV_INIT;
        batt_info.polling_time = polling_time > (5*HZ) ? polling_time : BATTERY_DEFAULT_POLL_TIME;
        batt_info.critical_polling_time = critical_polling_time > (3*HZ) ? 
                critical_polling_time : BATTERY_CRITICAL_POLL_TIME;
        batt_info.critical_polling_time = BATTERY_CRITICAL_POLL_TIME;
        batt_info.percentage = 50;
        batt_info.cable_status = pending_cable_status; 
        batt_info.cell_status = cell_status;
        batt_info.batt_temp = 250;
        batt_info.present = 1;
        batt_info.test_flag = test_flag;
        if (test_flag)
                BAT_DBG("test_flag: 0x%08X\n", test_flag);
        mutex_unlock(&batt_info_mutex);

#if CONFIG_PROC_FS
        ret = asus_battery_register_proc_fs();
        if (ret) {
                BAT_DBG_E("Unable to create proc file\n");
                goto proc_fail;
        }
#endif

        battery_work_queue = create_singlethread_workqueue("asus_battery");
        if(battery_work_queue == NULL)
        {
                BAT_DBG_E("Create battery thread fail");
                ret = -ENOMEM;
                goto error_workq;
        }
        INIT_DELAYED_WORK_DEFERRABLE(&battery_poll_data_work, asus_polling_data);
        INIT_DELAYED_WORK_DEFERRABLE(&detect_cable_work, USB_cable_status_worker);
        
        mutex_lock(&batt_info_mutex);
        batt_info.drv_status = DRV_INIT_OK;
        mutex_unlock(&batt_info_mutex);

        BAT_DBG("%s: success\n", __func__);
        return 0;

error_workq:    
#if CONFIG_PROC_FS
proc_fail:
#endif
already_init:
        return ret;
}
EXPORT_SYMBOL(asus_battery_init);

void asus_battery_exit(void)
{
        drv_status_t drv_sts;    

        BAT_DBG("Driver unload\n");

        mutex_lock(&batt_info_mutex);
        drv_sts = batt_info.drv_status;
        mutex_unlock(&batt_info_mutex);

        if (drv_sts == DRV_REGISTER_OK) {
                cancel_delayed_work_sync(&battery_poll_data_work);
                cancel_delayed_work_sync(&detect_cable_work);
                power_supply_unregister(&asus_power_supplies[CHARGER_BATTERY]);
                power_supply_unregister(&asus_power_supplies[CHARGER_USB]);
                power_supply_unregister(&asus_power_supplies[CHARGER_MAINS]);
        }

        drv_sts = DRV_SHUTDOWN;

        mutex_lock(&batt_info_mutex);
        batt_info.drv_status = drv_sts; 
        mutex_unlock(&batt_info_mutex);

        BAT_DBG("Driver unload OK\n");
}

static int __init asus_battery_fake_init(void)
{
        BAT_DBG("Date: "UTS_VERSION"\n");
        return 0;
}
late_initcall(asus_battery_fake_init);

static void __exit asus_battery_fake_exit(void)
{
        /* SHOULD NOT REACHE HERE */
        BAT_DBG("%s exit\n", __func__);
}
module_exit(asus_battery_fake_exit);

MODULE_AUTHOR("Wade1_Chen@asus.com");
MODULE_DESCRIPTION("battery driver");
MODULE_LICENSE("GPL");



