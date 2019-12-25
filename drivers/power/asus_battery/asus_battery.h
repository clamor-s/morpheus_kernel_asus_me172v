/*
 * Copyright (c) 2012, ASUSTek, Inc. All Rights Reserved.
 * Written by wade chen Wade1_Chen@asus.com
 */
#ifndef __ASUS_BATTERY_H__
#define __ASUS_BATTERY_H__

#include <mach/common_def.h>
#include <linux/i2c.h>
#include "bq27520_battery_upt_i2c.h"

typedef enum {
        NO_CABLE=0,     
        USB_ADAPTER,    /* usb plugg-in adapter */
        USB_PC,         /* usb plugg-in PC */
        NUM_CABLE_STATES,
} cable_type_t;

typedef enum {
        CHARGER_BATTERY = 0,
        CHARGER_USB,
        CHARGER_MAINS
} charger_type_t;

typedef enum {
        DRV_NOT_READY = 0,
        DRV_INIT,
        DRV_INIT_OK,
        DRV_REGISTER,
        DRV_REGISTER_OK,
        DRV_BATTERY_LOW,
        DRV_SHUTDOWN,
} drv_status_t;

typedef enum {
        DEV_NOT_READY = 0,
        DEV_INIT,
        DEV_INIT_OK,
} dev_status_t;

struct battery_dev_info {
        dev_status_t status;
        u32 test_flag;
        struct i2c_client *i2c;
        int update_status;
};

struct asus_bat_config {
        int battery_support;
        int polling_time;
        int critical_polling_time;
        int slave_addr;
};

struct asus_batgpio_set{ 
     int  active;  // pull up/down, or < 0 not support
     int  bitmap;   
     int  ctraddr;   
     int  icaddr;   
     int  idaddr;   
     int  ipcaddr;  
     int  ipdaddr;  
     int  int_active; //support interrupt or not. if < 0 mean not support.
     int  int_bitmap;
     int  int_sts_addr;
     int  int_ctrl_addr;
     int  int_type_addr;
     int  int_type_val_clr;
     int  int_type_val_set;
};  

//i2c device relative function
/*
 * Return an unsigned integer value of the predicted remaining battery capacity 
 * expressd as a percentage of FullChargeCapacity(), with a range of 0 to 100%.
 * Or < 0 if something fails.
 */
//int asus_battery_dev_read_percentage(void);
/*
 * Return the battery Voltage in milivolts
 * Or < 0 if something fails.
 */
//int asus_battery_dev_read_volt(void);
/*
 * Return the battery average current
 * Note that current can be negative signed as well(-65536 ~ 65535).
 * So, the value get from device need add a base(0x10000) to be a positive number if no error. 
 * Or < 0 if something fails.
 */
//int asus_battery_dev_read_current(void);
/*
 * Return an unsigned integer value of the predicted charge or energy remaining in 
 * the battery. The value is reported in units of mWh.
 * Or < 0 if something fails.
 */
//int asus_battery_dev_read_av_energy(void);
//
/*
 * Return the battery temperature in tenths of degree Celsius
 * Or < 0 if something fails.
 */
//int asus_battery_dev_read_temp(void);

//Collect functions as structure
struct dev_func {
        int (*read_percentage)(void);
        int (*read_volt)(void);
        int (*read_current)(void);
        int (*read_av_energy)(void);
        int (*read_temp)(void);
};


//power relative function, called some times by other drivers,
//but could only service one device.
int asus_battery_init(
        u32 polling_time, 
        u32 critical_polling_time, 
        u32 cell_status,
        u32 test_flag
);

void asus_battery_resume(void);
void asus_battery_suspend(void);
void asus_battery_exit(void);
int asus_register_power_supply(struct device *dev, struct dev_func *tbl);
int asus_battery_low_event(void);
u32 receive_USBcable_type(void);	//<ASUS-Darren1_Chang20121130+>
void update_battery_driver_state(void);
int get_battery_capacity(void);

#if defined(CONFIG_ASUS_ENGINEER_MODE) && CONFIG_ASUS_ENGINEER_MODE
extern int smb_charging_op(int do_charging);
#endif

struct battery_info_reply {
        u32 status;				/* Battery status */
        u32 health;				/* Battery health */
        u32 present;			/* Battery present */
        u32 technology;			/* Battery technology */
        u32 batt_volt;			/* Battery voltage */
        int batt_current;		/* Battery current */
        u32 batt_energy;		/* Battery energy */
        u32 percentage;			/* Battery percentage, -1 is unknown status */
        u32 level;				/* Battery level */
        int batt_temp;			/* Battery Temperature */
//        u32 charging_source;	/* 0: no cable, 1:usb, 2:AC */
//        u32 charging_enabled;	/* 0: Disable, 1: Enable */
//        u32 full_bat;			/* Full capacity of battery (mAh) */
        u32 polling_time;
        u32 critical_polling_time;
        u32 cable_status;
        u32 cell_status;        
                /*
                 *  BIT0 for 2 cell type.
                 *      the voltage level and cell type mapping is depend on battery team
                 */
        u32 test_flag;
                /*
                 * BIT0: seperate with other devices, 
                 *       other drivers should not register power supply class
                 * BIT1: not update forever
                 */
        drv_status_t        drv_status;				/* Battery driver status */
        struct dev_func *tbl;
};

#define TEST_INFO_NO_REG_POWER          BIT0 
#define TEST_INFO_DISABLE_PROTECT       BIT1
#define TEST_INFO_PROC_DUMP             BIT2

#define BATTERY_DEFAULT_POLL_TIME   (60 * HZ) //60 seconds
#define BATTERY_CRITICAL_POLL_TIME	(15 * HZ) //15 seconds

//#define BATTERY_PROC_NAME  "driver/virtual_battery"


#define BATTERY_TAG "<BATT>"
#define BAT_DBG(...)  printk(KERN_INFO BATTERY_TAG __VA_ARGS__);
#define BAT_DBG_L(level, ...)  printk(level BATTERY_TAG __VA_ARGS__);
#define BAT_DBG_E(...)  printk(KERN_ERR BATTERY_TAG __VA_ARGS__);

#define BATTERY_I2C_ANY_ADDR    0xFF



#endif //#define __ASUS_BATTERY_H__
