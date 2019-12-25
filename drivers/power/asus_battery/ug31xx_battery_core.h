/*
 * Copyright (c) 2012, ASUSTek, Inc. All Rights Reserved.
 * Written by wade chen Wade1_Chen@asus.com
 */
#ifndef __UG31XX_BATTERY_H__
#define __UG31XX_BATTERY_H__

#define UG31XX_I2C_BUS         3
#define UG31XX_DEV_NAME        "ug31xx-battery"
#define UG31XX_I2C_DEFAULT_ADDR 0x70 //7-bit
struct ug31xx_bat_platform_data {
        u32 i2c_bus_id;
};

/*
 * common property
 * if TEST_INFO_NO_REG_POWER is set in major flag, this bit will be set, too.
 * And also, mask out other test flag in minor test_flag.
 */
#define TEST_UG31_PROC_INFO_DUMP        BIT0

#define TEST_UG31_PROC_REG_DUMP         BIT1 
#define TEST_UG31_PROC_GGB_INFO_DUMP    BIT2
#define TEST_UG31_PROC_I2C_PROTOCOL     BIT3
#define TEST_UG31_PROC_ONE_SHOT         BIT4
#define TEST_UG31_PRINT                 BIT5

#endif //#define __UG31XX_BATTERY_H__

