/* alert_thread.c: receive POSIX messages and notify Logger thread.
 * Author: [fill in your name and student ID] | 2026-11-02
 */
#include "alert_thread.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

void *alert_thread_main(void *ptr)
{
    SmsContext *ctx = ptr;
    SmsAlert alert;
    char line[LOG_LINE_SIZE];
    logq_push(&ctx->logq, "Alert thread started");
    for (;;) {
        struct timespec limit;
        ssize_t n;
        clock_gettime(CLOCK_REALTIME, &limit);
        limit.tv_nsec += 250000000L;
        if (limit.tv_nsec >= 1000000000L) {
            limit.tv_nsec -= 1000000000L;
            limit.tv_sec++;
        }
        n = mq_timedreceive(ctx->alarm_mq, (char *)&alert, sizeof(alert), NULL, &limit);
        if (n == (ssize_t)sizeof(alert)) {
            fprintf(stderr, "[ALERT][%llu] TYPE: %s VALUE: %.1f THRESHOLD: %.1f\n",
                    alert.timestamp_ms, alert.type, alert.value, alert.threshold);
            snprintf(line, sizeof(line), "ALERT ts=%llu type=%s value=%.1f threshold=%.1f",
                     alert.timestamp_ms, alert.type, alert.value, alert.threshold);
            logq_push(&ctx->logq, line);
            if (strcmp(alert.type, "TEMP_HIGH") == 0)
                atomic_fetch_add(&ctx->temp_high, 1);
            else if (strcmp(alert.type, "HUMID_HIGH") == 0)
                atomic_fetch_add(&ctx->humid_high, 1);
            else if (strcmp(alert.type, "TEMP_SPIKE") == 0)
                atomic_fetch_add(&ctx->temp_spike, 1);
        } else if (n < 0 && errno != ETIMEDOUT && errno != EINTR) {
            perror("mq_timedreceive");
            atomic_store(&ctx->failed, 1);
            break;
        }
        if (atomic_load(&ctx->processor_done)) {
            struct mq_attr attr;
            if (mq_getattr(ctx->alarm_mq, &attr) == 0 && attr.mq_curmsgs == 0)
                break;
        }
    }
    logq_push(&ctx->logq, "Alert thread stopped");
    return NULL;
}
