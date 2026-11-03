/* processor_thread.c: rolling average, threshold detection, IPC alerts.
 * Author: [fill in your name and student ID] | 2026-11-02
 */
#include "processor_thread.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static void emit_alarm(SmsContext *ctx, const SensorData *s,
                       const char *type, double value, double threshold)
{
    SmsAlert a = { .timestamp_ms = s->timestamp_ms,
                   .value = value, .threshold = threshold };
    struct timespec deadline;
    snprintf(a.type, sizeof(a.type), "%s", type);
    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_nsec += 100 * 1000000L;
    if (deadline.tv_nsec >= 1000000000L) {
        deadline.tv_nsec -= 1000000000L;
        deadline.tv_sec++;
    }
    if (mq_timedsend(ctx->alarm_mq, (const char *)&a, sizeof(a), 0, &deadline) != 0)
        logq_push(&ctx->logq, "WARN: alarm_queue busy, alert dropped");
}

void *processor_thread_main(void *ptr)
{
    SmsContext *ctx = ptr;
    double temp_window[10] = {0}, hum_window[10] = {0};
    double temp_sum = 0, hum_sum = 0;
    double prev_temperature = 0;
    unsigned int count = 0;
    SensorData s;
    logq_push(&ctx->logq, "Processor thread started");
    while (cb_pop(&ctx->buffer, &s)) {
        unsigned int index = count % 10;
        unsigned int window_n = count < 10 ? count + 1 : 10;
        char logline[LOG_LINE_SIZE];
        if (count >= 10) {
            temp_sum -= temp_window[index];
            hum_sum -= hum_window[index];
        }
        temp_window[index] = s.temperature;
        hum_window[index] = s.humidity;
        temp_sum += s.temperature;
        hum_sum += s.humidity;
        snprintf(logline, sizeof(logline),
                 "SAMPLE ts=%llu t=%.1f h=%.1f avg10_t=%.2f avg10_h=%.2f",
                 s.timestamp_ms, s.temperature, s.humidity,
                 temp_sum / window_n, hum_sum / window_n);
        logq_push(&ctx->logq, logline);
        if (s.temperature > ctx->temp_threshold)
            emit_alarm(ctx, &s, "TEMP_HIGH", s.temperature, ctx->temp_threshold);
        if (s.humidity > ctx->hum_threshold)
            emit_alarm(ctx, &s, "HUMID_HIGH", s.humidity, ctx->hum_threshold);
        if (count > 0 && fabs(s.temperature - prev_temperature) > 5.0)
            emit_alarm(ctx, &s, "TEMP_SPIKE", fabs(s.temperature - prev_temperature), 5.0);
        prev_temperature = s.temperature;
        count++;
        atomic_fetch_add(&ctx->processed, 1);
    }
    logq_push(&ctx->logq, "Processor thread stopped");
    atomic_store(&ctx->processor_done, true);
    return NULL;
}
