/*
 * Copyright (c) 2012, ASUSTek, Inc. All Rights Reserved.
 * Written by wade chen Wade1_Chen@asus.com
 */
#include <linux/i2c.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/time.h>

#include "asus_battery.h"
#include "ug31xx_reg_def.h"
#include "ug31xx_i2c.h"
#include "ug31xx_typeDefine.h"
#include "ug31xx_api.h"
#include "ug31xx_proc_fs.h"
#include "ug31xx_battery_core.h"

#define PROC_DRV_PATH   "driver/ug31_battery"

extern struct battery_dev_info ug31_dev_info;
extern struct mutex ug31_dev_info_mutex;

extern  CELL_TABLE      cellTable;
extern  CELL_PARAMETER  cellParameter;
extern struct mutex  ug31_gauge_mutex;

//only support one ggb item. No multi-file.
static u8 ggbx_file_buf[8*1024]; //save input buffer

int prepare_ggbx_file_buf(void)
{
        GGBX_FILE_HEADER *p_ggbx = (GGBX_FILE_HEADER *)ggbx_file_buf;
        struct timeval tv;
        u16 sum16;
        u8 *p_start=NULL, *p_end=NULL;

        p_start = ggbx_file_buf + sizeof(GGBX_FILE_HEADER);
        mutex_lock(&ug31_dev_info_mutex);    
        memcpy(p_start, &cellParameter, sizeof(CELL_PARAMETER));
        memcpy(p_start + sizeof(CELL_PARAMETER), &cellTable, sizeof(CELL_TABLE));
        mutex_unlock(&ug31_dev_info_mutex);

        p_ggbx->ggb_tag = GGBX_FILE_TAG;
        do_gettimeofday(&tv);
        p_ggbx->time_stamp = tv.tv_sec;
        p_ggbx->length = sizeof(CELL_PARAMETER) + sizeof(CELL_TABLE);
        p_ggbx->num_ggb = 1;
        /*
         * calculate checksum
         */
        p_start = (u8 *)p_ggbx + sizeof(GGBX_FILE_HEADER);
        p_end = p_start + sizeof(CELL_PARAMETER) + sizeof(CELL_TABLE)-1;
        for (sum16=0; p_start <= p_end; p_start++) {
                sum16 += *p_start;
        }

        printk("cell parameter %d, cell table %d\n", sizeof(CELL_PARAMETER), sizeof(CELL_TABLE));
        p_ggbx->sum16 = sum16;

        return 0;
}

int reinstall_ggb(void)
{
        GGBX_FILE_HEADER *p_ggbx = (GGBX_FILE_HEADER *)ggbx_file_buf;
        u8 *p_start=NULL, *p_end=NULL;
        u16 sum16=0;
        int ret=0;

        /* Check header */
        if (p_ggbx->ggb_tag != GGBX_FILE_TAG) {
                BAT_DBG_E("[%s] tag(0x%08X) error in .ggb file.\n", 
                        __func__, p_ggbx->ggb_tag);
                return -EFAULT;
        }

        if (p_ggbx->length < sizeof(CELL_PARAMETER) + sizeof(CELL_TABLE)) {
                BAT_DBG_E("[%s] length(%d) error in .ggb file.\n", 
                        __func__, (u32)p_ggbx->length);
                return -EFAULT;
        }

        if (p_ggbx->num_ggb != 1) {
                BAT_DBG_E("[%s] num_ggb(%d) error in .ggb file.\n", 
                        __func__, (u32)p_ggbx->num_ggb);
                return -EFAULT;
        }

        p_start = ggbx_file_buf + sizeof(GGBX_FILE_HEADER);
        p_end = p_start + p_ggbx->length - 1;
        for (sum16=0; p_start <= p_end; p_start++) {
                sum16 += *p_start;
        }

        if (sum16 != p_ggbx->sum16) {
                BAT_DBG_E("[%s] checksum error in .ggb file. 0x%04X, 0x%04X\n", 
                        __func__, p_ggbx->sum16, sum16);
                return -EFAULT;
        }

        /*TODO: check cell type by sku */

        /* Check OK. start to reload */
        ret = upiGG_Initial(p_ggbx);
        if (ret != UG_INIT_SUCCESS) {
                BAT_DBG_E("[%s] Re-initial upi driver fail.\n", __func__); 
                return -EFAULT;
        }
        return 0;
}

int ug31_ggb_open(struct inode *ino, struct file *fp)
{
        /* Mode checking
         * Only support FMODE_WRITE, FMODE_READ, 
         *  not include (FMODE_WRITE | FMODE_READ)
         */
        if (!(fp->f_mode & (FMODE_WRITE | FMODE_READ)))
                return -EPERM;

        if ((fp->f_mode & (FMODE_WRITE | FMODE_READ)) == (FMODE_WRITE | FMODE_READ))
                return -EPERM;

        /* OK. Clean buffer */
        memset(ggbx_file_buf, 0x00, sizeof(ggbx_file_buf));

        /* If for reading, prepare ggbx file to buffer */
        if (fp->f_mode & FMODE_WRITE) 
                return 0;

        //prepare reading
        prepare_ggbx_file_buf();


        return 0;
}

ssize_t ug31_ggb_read(struct file *fp, char __user *buf, size_t count, loff_t *off)
{
        GGBX_FILE_HEADER *p_ggbx = (GGBX_FILE_HEADER *)ggbx_file_buf;
        u32 total_size = sizeof(GGBX_FILE_HEADER) + p_ggbx->length;

        /* boundary checking */
        if (*off >= total_size) 
                return 0;

        /* read count adjust if needed */
        if (*off + count > total_size)
                count = total_size - *off;

        /* transfer ... */
        if (copy_to_user(buf, ggbx_file_buf + *off, count))
                return -EFAULT;

        *off += count;

        return count;
}

ssize_t ug31_ggb_write(struct file *fp, const char __user *buf, size_t count, loff_t *off)
{
        /* boundary checking */
        if (*off >= sizeof(ggbx_file_buf)) 
                return -EIO;

        if (*off+count > sizeof(ggbx_file_buf)) 
                count = sizeof(ggbx_file_buf) - *off;

        /* transfer ... */
        if (copy_from_user(ggbx_file_buf+*off, buf, count))
                return -EFAULT;

        *off += count;

        return count;
}

int ug31_ggb_release(struct inode *ino, struct file *fp)
{
        int ret=0;

        if (fp->f_mode & FMODE_WRITE) {
                ret = reinstall_ggb(); 
        }
        /* Clean buffer again */
        memset(ggbx_file_buf, 0x00, sizeof(ggbx_file_buf));

        return ret;
}

static const struct file_operations ggb_file_proc_fops = {
        .owner	 = THIS_MODULE,
        .open    = ug31_ggb_open,
        .read    = ug31_ggb_read,
        .write   = ug31_ggb_write,
        .release = ug31_ggb_release,
};

int ug31xx_register_proc_fs(void)
{
        if (proc_create(PROC_DRV_PATH, 0666, NULL, &ggb_file_proc_fops) == NULL) {
                BAT_DBG_E("Unable to create" PROC_DRV_PATH "\n");
                return -EINVAL;
        }
#if defined(CONFIG_ASUS_ENGINEER_MODE) && CONFIG_ASUS_ENGINEER_MODE
        if (ug31xx_register_proc_fs_test()) {
                return -EINVAL;
        }
#endif
        return 0;
}
