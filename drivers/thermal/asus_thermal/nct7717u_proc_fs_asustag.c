/*
 * Copyright (c) 2012, ASUSTek, Inc. All Rights Reserved.
 * Written by wade chen Wade1_Chen@asus.com
 */
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <mach/hardware.h>

#include "nct7717u.h"

extern struct thermal_dev_info nct7717u_dev_info;
extern struct mutex nct7717u_dev_info_mutex;

static char  proc_buf[8*1024];
static int return_val;

#define BQ_DUMP(...) \
do { \
        local_len = sprintf(page, __VA_ARGS__); \
        len += local_len; \
        page += local_len; \
}while(0);

int nct7717u_proc_protocol_read(char *page, char **start, off_t off, int count, int *eof, void *date)
{
        int len; 
        
        len = sprintf(page, "0x%08X\n", return_val);

        return len;
}

int nct7717u_proc_protocol_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
        int reg_off=0, value=0, ret=0;
        char mode, len;
        int byte_len=1;
        static struct i2c_client *client=NULL;

        mutex_lock(&nct7717u_dev_info_mutex);    
        client = nct7717u_dev_info.i2c;
        mutex_unlock(&nct7717u_dev_info_mutex);

        dev_info(&client->dev, "%s\n", __func__);

        if (count > sizeof(proc_buf)) {
                dev_err(&client->dev, "data error\n");
                return_val = -EINVAL;
                return -EINVAL;
        }

        if (copy_from_user(proc_buf, buffer, count)) {
                dev_err(&client->dev, "read data from user space error\n");
                return_val = -EFAULT;
                return -EFAULT;
        }

        sscanf(proc_buf, "%c%c %02X %02X\n", &mode, &len, &reg_off, &value);
        if (len == 'b') {
                byte_len = 1;
        }else if (len == 'w') {
                byte_len = 2;
        }

        if (mode == 'w') {
                dev_info(&client->dev, "%s write reg:%02X, value: %04X, len: %d\n", 
                                __func__, reg_off, value, byte_len);
                ret = nct7717u_write_i2c(client, reg_off, value, byte_len % 2);

        } else if (mode == 'r') {
                ret = nct7717u_read_i2c(client, reg_off, &value, byte_len % 2);
                dev_info(&client->dev, "%s read reg:%02X, value: %04X, len: %d\n", 
                                __func__, reg_off, value, byte_len);
                return_val = value;

        } else if (mode == 's' && len == 't') {
                mutex_lock(&nct7717u_dev_info_mutex);    
                //only +127oC ~ -128oC
                if (reg_off & BIT7) {
                    printk("davidwangs nct7717u2 register %d\n",reg_off);
                    reg_off = 0;
                        /*reg_off &= (BIT7 - 1);*/
                        /*reg_off = -reg_off;*/
                }
                nct7717u_dev_info.replace_temp = reg_off;
                mutex_unlock(&nct7717u_dev_info_mutex);
        }else {
                dev_info(&client->dev, "mode fail\n");
                return_val = -EACCES;
        }

        if (ret) {
                dev_err(&client->dev, "i2c operation  fail. %d\n", ret);
                return_val = -EIO;
                return -EIO;
        }

        memset(proc_buf, 0x00, sizeof(proc_buf));

        return count;
}


int nct7717u_register_proc_fs_test(void)
{
        struct proc_dir_entry *entry=NULL;
        struct thermal_dev_info tmp_dev_info;

        mutex_lock(&nct7717u_dev_info_mutex);    
        tmp_dev_info = nct7717u_dev_info;
        mutex_unlock(&nct7717u_dev_info_mutex);

        entry = create_proc_entry("nct7717u_test_proc_protocol", 0666, NULL);
        if (!entry) {
                TM_DBG_E("Unable to create nct7717u_test_proc_protocol\n");
                return -EINVAL;
        }
        entry->read_proc = nct7717u_proc_protocol_read;
        entry->write_proc = nct7717u_proc_protocol_write;
        
        return 0;
}
