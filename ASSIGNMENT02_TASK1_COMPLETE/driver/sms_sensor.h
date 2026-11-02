#ifndef SMS_SENSOR_H
#define SMS_SENSOR_H

#include <linux/ioctl.h>

#define SMS_DEVICE_NAME  "sms_sensor"
#define SMS_DEVICE_PATH  "/dev/sms_sensor"
#define SMS_PROC_NAME    "sms_stats"
#define SMS_IOC_MAGIC   'S'
#define SMS_SET_INTERVAL _IOW(SMS_IOC_MAGIC, 1, unsigned int)

#endif /* SMS_SENSOR_H */
