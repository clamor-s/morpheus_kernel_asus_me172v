/*
 * Copyright (c) 2012, ASUSTek, Inc. All Rights Reserved.
 * Written by wade chen Wade1_Chen@asus.com
 */
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#include "asus_battery.h"

#define PROCFS_BATTERY 		"battery_status"

extern int wmt_getsyspara(char *varname, char *varval, int *varlen);
extern int wmt_setsyspara(char *varname, char *varval);

extern struct mutex batt_info_mutex;
extern struct battery_info_reply batt_info; 

static char  proc_buf[8*1024];

int asus_battery_info_proc_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
        return count;
}

char str_batt_status[5][40] = {"POWER_SUPPLY_STATUS_UNKNOWN", 
                                "POWER_SUPPLY_STATUS_CHARGING", 
                                "POWER_SUPPLY_STATUS_DISCHARGING", 
                                "POWER_SUPPLY_STATUS_NOT_CHARGING", 
                                "POWER_SUPPLY_STATUS_FULL"};

char str_batt_level[6][40] = {"POWER_SUPPLY_CAPACITY_LEVEL_UNKNOWN",
                                "POWER_SUPPLY_CAPACITY_LEVEL_CRITICAL",
                                "POWER_SUPPLY_CAPACITY_LEVEL_LOW",
                                "POWER_SUPPLY_CAPACITY_LEVEL_NORMAL",
                                "POWER_SUPPLY_CAPACITY_LEVEL_HIGH",
                                "POWER_SUPPLY_CAPACITY_LEVEL_FULL"};

char str_batt_cable_sts[][20] = {
        "NO_CABLE", 
        "USB_ADAPTER", 
        "USB_PC"
};

char str_batt_drv_sts[][30] = {
        "DRV_NOT_READY", 
        "DRV_INIT", 
        "DRV_INIT_OK", 
        "DRV_REGISTER",
        "DRV_REGISTER_OK",
        "DRV_BATTERY_LOW",
        "DRV_SHUTDOWN",
};

char str_batt_cell_status[][30] = {
        "GPIO_L",
        "GPIO_H"
};

int asus_battery_info_proc_read(char *page, char **start, off_t off, int count, int *eof, void *date)
{
        int len=0;
        struct battery_info_reply tmp_batt_info;

        mutex_lock(&batt_info_mutex);
        tmp_batt_info = batt_info;
        mutex_unlock(&batt_info_mutex);

        len = sprintf(page, 
                        "battery_status=%s\n"
                        "battery_voltage=%d\n"
                        "battery_current=%d\n"
                        "battery_available_energy=%d\n"
                        "percentage=%d%%\n"
                        "level=%s\n"
                        "battery_temperature=%d\n"
                        "polling_time=%d\n"
                        "critical_polling_time=%d\n"
                        "cable_status=%s\n"
                        "cell_status=%s\n"
                        "driver_status=%s\n",
                        str_batt_status[tmp_batt_info.status],
                        tmp_batt_info.batt_volt,
                        tmp_batt_info.batt_current,
                        tmp_batt_info.batt_energy,
                        tmp_batt_info.percentage,
                        str_batt_level[tmp_batt_info.level],
                        tmp_batt_info.batt_temp,
                        tmp_batt_info.polling_time,
                        tmp_batt_info.critical_polling_time,
                        str_batt_cable_sts[tmp_batt_info.cable_status],
                        str_batt_cell_status[tmp_batt_info.cell_status],
                        str_batt_drv_sts[tmp_batt_info.drv_status]
                                );

        return len;
}

int asus_battery_profile_read(char *page, char **start, off_t off, int count, int *eof, void *date)
{
	int ret;
	unsigned int current_percent = 0;
        struct battery_info_reply tmp_batt_info;

        mutex_lock(&batt_info_mutex);
        tmp_batt_info = batt_info;
        mutex_unlock(&batt_info_mutex);

	current_percent = tmp_batt_info.percentage;
	ret = sprintf(page, "%d\n", current_percent);

	return ret;
}

static char proc_uboot_var_name[64];
static char proc_uboot_var_value[256];
static int proc_uboot_var_buf_size = sizeof(proc_uboot_var_value); 
int asus_bat_proc_config_var_read(char *page, char **start, off_t off, int count, int *eof, void *date)
{

        int len=0 ;

        len = sprintf(page, "%s %s\n", proc_uboot_var_name, proc_uboot_var_value);

        return len;
}

int asus_bat_proc_config_var_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
        memset(proc_uboot_var_name, 0, sizeof(proc_uboot_var_name));
        memset(proc_uboot_var_value, 0, sizeof(proc_uboot_var_value));
        memset(proc_buf, 0, sizeof(proc_buf));
        

        if (count > sizeof(proc_buf)) {
                BAT_DBG_E("%s: Input data error\n", __func__);
                return -EINVAL;
        }

        if (copy_from_user(proc_buf, buffer, count)) {
                BAT_DBG_E("%s: copy data error\n", __func__);
                return -EFAULT;
        }

        sscanf(proc_buf, "%s %s\n", proc_uboot_var_name, proc_uboot_var_value);
        if (!strlen(proc_uboot_var_value)) {
                //BAT_DBG_E("%s %s\n", proc_uboot_var_name, proc_uboot_var_value);
                if (wmt_getsyspara(proc_uboot_var_name, proc_uboot_var_value, &proc_uboot_var_buf_size)) {
                        BAT_DBG_E("%s: read var value fail\n", __func__);
                } else {
                        BAT_DBG_E("[%s] read %s = %s \n", __func__, 
                                        proc_uboot_var_name, proc_uboot_var_value);
                }

        } else {
                BAT_DBG_E("[%s] write %s = %s \n", __func__, 
                                proc_uboot_var_name, proc_uboot_var_value);

                if (wmt_setsyspara(proc_uboot_var_name, proc_uboot_var_value)) {
                        BAT_DBG_E("%s: setting fail\n", __func__);
                }
        }


        return count;
}

int asus_battery_register_proc_fs_test(void)
{
        struct proc_dir_entry *entry=NULL;
        struct battery_info_reply tmp_batt_info;

        mutex_lock(&batt_info_mutex);
        tmp_batt_info = batt_info;
        mutex_unlock(&batt_info_mutex);

        entry = create_proc_entry(PROCFS_BATTERY, 0666, NULL);
        if (!entry) {
                BAT_DBG_E("Unable to create %s\n", PROCFS_BATTERY);
                return -EINVAL;
        }
        entry->read_proc = asus_battery_profile_read;

        //always exist for config var.
        entry = create_proc_entry("asus_battery_cfg_var", 0666, NULL);
        if (!entry) {
                BAT_DBG_E("Unable to create asus_battery_cfg_var\n");
                return -EINVAL;
        }
        entry->read_proc = asus_bat_proc_config_var_read;
        entry->write_proc = asus_bat_proc_config_var_write;

        entry = create_proc_entry("asus_battery_info", 0666, NULL);
        if (!entry) {
                BAT_DBG_E("Unable to create asus_battery_info\n");
                return -EINVAL;
        }
        entry->read_proc = asus_battery_info_proc_read;
        entry->write_proc = asus_battery_info_proc_write;


        return 0;
}
