/*
 * Copyright (c) 2012, ASUSTek, Inc. All Rights Reserved.
 * Written by wade chen Wade1_Chen@asus.com
 */
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#include "nct7717u_proc_fs.h" 

int nct7717u_register_proc_fs(void)
{
        int ret=0;

#if defined(CONFIG_ASUS_ENGINEER_MODE) && CONFIG_ASUS_ENGINEER_MODE
        ret = nct7717u_register_proc_fs_test();
#endif
        return ret;
}
