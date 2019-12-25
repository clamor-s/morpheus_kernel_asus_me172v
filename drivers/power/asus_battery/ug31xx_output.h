#define LOG_NORMAL      0
#define LOG_ERROR       1
#define LOG_DATA        2
#define LOG_VERBOSE     3

#define UG31_TAG "UG31"
#define UG31_LOGE(...)  ug31_printk(LOG_ERROR, "<"UG31_TAG"/E>"  __VA_ARGS__);
#define UG31_LOGI(...)  ug31_printk(LOG_NORMAL, "<"UG31_TAG"/I>"  __VA_ARGS__);
#define UG31_LOGD(...)  ug31_printk(LOG_DATA, "<"UG31_TAG"/D>"  __VA_ARGS__);
#define UG31_LOGV(...)  ug31_printk(LOG_VERBOSE, "<"UG31_TAG"/V>"  __VA_ARGS__);


int ug31_printk(int level, const char *fmt, ...); 
