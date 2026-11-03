/* sensor_thread.c: periodic sensor acquisition and buffer producer.
 * Author: [fill in your name and student ID] | 2026-11-02
 */
#include "sensor_thread.h"
#include "../driver/sms_sensor.h"
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static void advance(struct timespec *t, int interval_ms)
{
    t->tv_nsec += (long)(interval_ms % 1000) * 1000000L;
    t->tv_sec += interval_ms / 1000 + t->tv_nsec / 1000000000L;
    t->tv_nsec %= 1000000000L;
}

static int read_sensor(SmsContext *ctx, SensorData *sample)
{
    char csv[128];
    int fd, attempt;
    ssize_t n;
    for (attempt = 0; attempt < 10 && !atomic_load(&ctx->stop); attempt++) {
        fd = open(ctx->device_path, O_RDONLY | O_CLOEXEC);
        if (fd < 0)
            return -1;
        n = read(fd, csv, sizeof(csv) - 1);
        /* Preserve errno across close(). */
        int read_errno = errno;
        close(fd);
        if (n > 0) {
            csv[n] = '\0';
            if (sscanf(csv, "%llu,%lf,%lf", &sample->timestamp_ms,
                       &sample->temperature, &sample->humidity) == 3)
                return 0;
            errno = EPROTO;
            return -1;
        }
        if (n == 0) {
            errno = EAGAIN;
        } else {
            errno = read_errno;
        }
        if (errno != EAGAIN && errno != EINTR)
            return -1;
        struct timespec delay = { .tv_sec = 0, .tv_nsec = 2000000L };
        nanosleep(&delay, NULL);
    }
    errno = EAGAIN;
    return -1;
}

void *sensor_thread_main(void *ptr)
{
    SmsContext *ctx = ptr;
    struct timespec deadline;
    int policy = ctx->sched_fifo ? SCHED_FIFO : SCHED_OTHER;
    struct sched_param param = { .sched_priority = ctx->sched_fifo ? 50 : 0 };
    int policy_error = pthread_setschedparam(pthread_self(), policy, &param);
    if (policy_error)
        fprintf(stderr, "[sensor] requested %s unavailable (%s); fallback in effect\n",
                ctx->sched_fifo ? "SCHED_FIFO" : "SCHED_OTHER", strerror(policy_error));
    else
        fprintf(stderr, "[sensor] %s priority %d enabled\n",
                ctx->sched_fifo ? "SCHED_FIFO" : "SCHED_OTHER", param.sched_priority);
    logq_push(&ctx->logq, "Sensor thread started");
    clock_gettime(CLOCK_MONOTONIC, &deadline);
    while (!atomic_load(&ctx->stop)) {
        SensorData sample;
        if (read_sensor(ctx, &sample) == 0) {
            atomic_fetch_add(&ctx->samples_collected, 1);
            if (!cb_push(&ctx->buffer, sample)) {
                logq_push(&ctx->logq, "WARN: shared_buffer full, sample dropped");
            } else if (ctx->verbose) {
                printf("[SENSOR] %llu %.1fC %.1f%%\n", sample.timestamp_ms,
                       sample.temperature, sample.humidity);
                fflush(stdout);
            }
        } else if (errno != EAGAIN && errno != EINTR) {
            char warning[LOG_LINE_SIZE];
            snprintf(warning, sizeof(warning), "WARN: sensor read failed: %s", strerror(errno));
            logq_push(&ctx->logq, warning);
            /* Missing device is a fatal integration problem. */
            if (errno == ENOENT || errno == ENODEV) {
                atomic_store(&ctx->failed, 1);
                atomic_store(&ctx->stop, true);
                break;
            }
        }
        advance(&deadline, ctx->interval_ms);
        while (!atomic_load(&ctx->stop)) {
            int rc = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &deadline, NULL);
            if (rc == 0 || rc != EINTR)
                break;
        }
        /* Signed wake-up error (microseconds), measured after the absolute sleep.
         * Only Sensor Thread writes these entries; main reads them after join(). */
        if (!atomic_load(&ctx->stop) && ctx->jitter_count < SMS_JITTER_MAX) {
            struct timespec actual;
            if (clock_gettime(CLOCK_MONOTONIC, &actual) == 0) {
                long long dt_us =
                    ((long long)actual.tv_sec - (long long)deadline.tv_sec) * 1000000LL +
                    ((long long)actual.tv_nsec - (long long)deadline.tv_nsec) / 1000LL;
                ctx->jitter_us[ctx->jitter_count++] = (long)dt_us;
            }
        }
    }
    logq_push(&ctx->logq, "Sensor thread stopped");
    return NULL;
}
