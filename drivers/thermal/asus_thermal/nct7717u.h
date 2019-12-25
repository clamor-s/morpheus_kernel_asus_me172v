/*
 * Copyright (c) 2012, ASUSTek, Inc. All Rights Reserved.
 * Written by wade chen Wade1_Chen@asus.com
 */
#ifndef __NCT7717U_H__
#define __NCT7717U_H__

#define NCT7717U_DIODE_TEMP             0x0000
#define NCT7717U_ALERT_STATUS           0x0002
#define NCT7717U_CONFIG                 0x0003
#define NCT7717U_CONV_RATE              0x0004
#define NCT7717U_CONV_RATE_W            0x000A
#define NCT7717U_ALERT_TEMP             0x0005
#define NCT7717U_ALERT_TEMP_W           0x000B
#define NCT7717U_ONE_SHOT_CONV          0x000F
#define NCT7717U_DATA_LOG_0             0x002D
#define NCT7717U_DATA_LOG_1             0x002E
#define NCT7717U_DATA_LOG_2             0x002F
#define NCT7717U_ALERT_MODE             0x00BF
#define NCT7717U_CHIP_ID                0x00FD
#define NCT7717U_VENDOR_ID              0x00FE
#define NCT7717U_DEV_ID                 0x00FF

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


struct asus_thermal_config {
        int thermal_support;
        int polling_time;
        int slave_addr;
};

struct thermal_dev_info {
        dev_status_t status;
        u32 test_flag;
        struct i2c_client *i2c;
        u32 mode;
        int replace_temp;
	struct thermal_zone_device *tz_dev;
        struct asus_thermal_config tm_cfg;
};

int nct7717u_write_i2c(struct i2c_client *client, u8 reg, int value, int b_single);
int nct7717u_read_i2c(struct i2c_client *client, u8 reg, int *rt_value, int b_single);

struct nct7717u_platform_data {
        u32 i2c_bus_id;
};

#define M_TEMP(x) (x*1000)

#define THERMAL_DEFAULT_POLL_TIME 10 // 1-second unit


#define NCT7717U_DEFAULT_ADDR 0x48

#define THERMAL_TAG "<THERMAL>"
#define TM_DBG(...)  printk(KERN_INFO THERMAL_TAG __VA_ARGS__);
#define TM_DBG_L(level, ...)  printk(level THERMAL_TAG __VA_ARGS__);
#define TM_DBG_E(...)  printk(KERN_ERR THERMAL_TAG __VA_ARGS__);

#define NCT7717U_I2C_BUS 0
#define NCT7717U_DEV_NAME "nct7717u"

#endif //ifndef __NCT7717U_H__
