#include <linux/module.h>
#include <linux/param.h>

#include "ug31xx_output.h"
#include "asus_battery.h"
#include "ug31xx_battery_core.h"

static u32 test_flag = 0xFFFFFFFF; 

extern struct battery_dev_info ug31_dev_info;
extern struct mutex ug31_dev_info_mutex;

int ug31_printk(int level, const char *fmt, ...) 
{
        va_list args;
        int r;

        if (test_flag == 0xFFFFFFFF) {
                mutex_lock(&ug31_dev_info_mutex);    
                test_flag = ug31_dev_info.test_flag;
                mutex_unlock(&ug31_dev_info_mutex);
        }

        if (test_flag & TEST_UG31_PRINT) {
                va_start(args, fmt);
                r = vprintk(fmt, args);
                va_end(args);
        }
     
        return r;
}
