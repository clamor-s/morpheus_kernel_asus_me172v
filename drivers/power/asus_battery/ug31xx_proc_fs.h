/*
 * Copyright (c) 2012, ASUSTek, Inc. All Rights Reserved.
 * Written by wade chen Wade1_Chen@asus.com
 */
#ifndef __UG31XX_PROC_FS_H__
#define __UG31XX_PROC_FS_H__

int ug31xx_register_proc_fs(void);

#if defined(CONFIG_ASUS_ENGINEER_MODE) && CONFIG_ASUS_ENGINEER_MODE
int ug31xx_register_proc_fs_test(void);
#endif


#endif //__UG31XX_PROC_FS_H__
