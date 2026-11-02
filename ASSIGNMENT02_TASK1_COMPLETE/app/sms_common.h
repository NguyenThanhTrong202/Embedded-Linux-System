#ifndef SMS_COMMON_H
#define SMS_COMMON_H
#include <stdatomic.h>
#include <stdbool.h>
#include <mqueue.h>
#include <pthread.h>
#include <stdint.h>
#include "circular_buffer.h"
#include "logger_thread.h"

#define SMS_DEFAULT_DEVICE "/dev/sms_sensor"
#define SMS_LOG_PATH "/tmp/sms.log"
#define SMS_BUFFER_CAPACITY 64

typedef struct {
    unsigned long long timestamp_ms;
    char type[20];
    double value;
    double threshold;
} SmsAlert;

typedef struct {
    char device_path[256];
    int interval_ms;
    double temp_threshold, hum_threshold;
    int duration_s, verbose;
    CircularBuffer buffer;
    LogQueue logq;
    mqd_t alarm_mq;
    char mq_name[64];
    atomic_bool stop;
    atomic_bool processor_done;
    atomic_uint samples_collected;
    atomic_uint processed;
    atomic_uint temp_high;
    atomic_uint humid_high;
    atomic_uint temp_spike;
    atomic_int failed;
} SmsContext;

void *sensor_thread_main(void *ctx);
void *processor_thread_main(void *ctx);
void *alert_thread_main(void *ctx);
#endif
