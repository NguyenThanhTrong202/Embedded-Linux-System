/* sms_app.c: ARMv7 sensor monitoring app with four POSIX threads.
 * Author: [fill in your name and student ID] | 2026-11-02
 */
#include "sms_common.h"
#include "../driver/sms_sensor.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

/* All samples are read after pthread_join(sensor), so no locking is required. */
static int cmp_long(const void *left, const void *right)
{
    long a = *(const long *)left, b = *(const long *)right;
    return (a > b) - (a < b);
}

static void report_jitter(const SmsContext *ctx)
{
    size_t n = ctx->jitter_count;
    long *sorted;
    double total = 0.0;
    size_t hist[4] = {0, 0, 0, 0};
    size_t p99_index;
    if (!n) {
        puts("JITTER samples=0 (no valid wakeups)");
        return;
    }
    sorted = malloc(n * sizeof(*sorted));
    if (!sorted) {
        fputs("JITTER: sorting allocation failed\n", stderr);
        return;
    }
    for (size_t i = 0; i < n; ++i) {
        long v = ctx->jitter_us[i];
        long nonnegative = v < 0 ? 0 : v;
        sorted[i] = v;
        total += (double)v;
        if (nonnegative < 100) hist[0]++;
        else if (nonnegative < 500) hist[1]++;
        else if (nonnegative < 1000) hist[2]++;
        else hist[3]++;
    }
    qsort(sorted, n, sizeof(*sorted), cmp_long);
    /* nearest-rank P99 = sorted[ceil(0.99 * N) - 1]. */
    p99_index = (99 * n + 99) / 100;
    if (p99_index) p99_index--;
    printf("JITTER policy=%s mlock=%s samples=%zu min_us=%ld avg_us=%.2f max_us=%ld p99_us=%ld\n",
           ctx->sched_fifo ? "FIFO" : "OTHER", ctx->mlock_ok ? "OK" : "FAILED",
           n, sorted[0], total / (double)n, sorted[n - 1], sorted[p99_index]);
    printf("JITTER_HIST 0-100us=%zu 100-500us=%zu 500-1000us=%zu >1ms=%zu\n",
           hist[0], hist[1], hist[2], hist[3]);
    if (n < 1000)
        fprintf(stderr, "WARNING: only %zu samples; Task 4 requires at least 1000.\n", n);
    free(sorted);
}

/* Bound each worker stack to 256 KiB so mlockall(MCL_FUTURE) will not
 * exhaust an 8 MiB memlock limit with four 8 MiB stacks. */
static int start_worker(pthread_t *tid, void *(*fn)(void *), void *arg)
{
    pthread_attr_t attr;
    int rc = pthread_attr_init(&attr);
    if (rc) return rc;
    rc = pthread_attr_setstacksize(&attr, 256U * 1024U);
    if (!rc) rc = pthread_create(tid, &attr, fn, arg);
    pthread_attr_destroy(&attr);
    return rc;
}

static volatile sig_atomic_t got_sigint;
static void handle_sigint(int signo) { (void)signo; got_sigint = 1; }

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s [-i ms] [-t Celsius] [-h percent] [-d seconds] [-v] [-p device] [-s fifo|other]\n", prog);
    fprintf(stderr, "Defaults: -i 500 -t 40.0 -h 80.0 -d 60, device=%s\n", SMS_DEFAULT_DEVICE);
}

static int configure_driver(const SmsContext *ctx, int custom_path)
{
    unsigned int interval = (unsigned int)ctx->interval_ms;
    int fd = open(ctx->device_path, O_RDONLY | O_CLOEXEC);
    if (fd < 0)
        return -1;
    if (ioctl(fd, SMS_SET_INTERVAL, &interval) != 0) {
        /* For a host-only smoke test, -p may point at a regular CSV file. */
        if (!(custom_path && errno == ENOTTY)) {
            int err = errno;
            close(fd);
            errno = err;
            return -1;
        }
        fprintf(stderr, "[demo] regular CSV input, ioctl unavailable\n");
    }
    close(fd);
    return 0;
}

int main(int argc, char **argv)
{
    SmsContext ctx = {0};
    pthread_t sensor, processor, alert, logger;
    int opt, custom_path = 0, stage = 0, exit_code = 0;
    struct sigaction action = {0};
    struct mq_attr attr = {0};
    struct timespec start, now, nap = { .tv_sec = 0, .tv_nsec = 100000000L };
    BufferStats stats;

    ctx.interval_ms = 500;
    ctx.temp_threshold = 40.0;
    ctx.hum_threshold = 80.0;
    ctx.duration_s = 60;
    ctx.sched_fifo = 1;
    snprintf(ctx.device_path, sizeof(ctx.device_path), "%s", SMS_DEFAULT_DEVICE);
    while ((opt = getopt(argc, argv, "i:t:h:d:vp:s:")) != -1) {
        char *end = NULL;
        switch (opt) {
        case 'i':
            ctx.interval_ms = (int)strtol(optarg, &end, 10);
            if (*end || ctx.interval_ms < 1 || ctx.interval_ms > 60000) goto bad_args;
            break;
        case 't':
            ctx.temp_threshold = strtod(optarg, &end);
            if (*end || !(ctx.temp_threshold >= 0.0 && ctx.temp_threshold <= 100.0)) goto bad_args;
            break;
        case 'h':
            ctx.hum_threshold = strtod(optarg, &end);
            if (*end || !(ctx.hum_threshold >= 0.0 && ctx.hum_threshold <= 100.0)) goto bad_args;
            break;
        case 'd':
            ctx.duration_s = (int)strtol(optarg, &end, 10);
            if (*end || ctx.duration_s < 0) goto bad_args;
            break;
        case 'v': ctx.verbose = 1; break;
        case 's':
            if (strcmp(optarg, "fifo") == 0) ctx.sched_fifo = 1;
            else if (strcmp(optarg, "other") == 0) ctx.sched_fifo = 0;
            else goto bad_args;
            break;
        case 'p':
            if (strlen(optarg) >= sizeof(ctx.device_path)) goto bad_args;
            snprintf(ctx.device_path, sizeof(ctx.device_path), "%s", optarg);
            custom_path = 1;
            break;
        default: goto bad_args;
        }
    }
    if (optind != argc) goto bad_args;
    if (configure_driver(&ctx, custom_path) != 0) {
        perror("configure /dev/sms_sensor");
        return 1;
    }
    action.sa_handler = handle_sigint;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGINT, &action, NULL) != 0) {
        perror("sigaction");
        return 1;
    }
    /* Lock memory before creating worker threads; continue if privileges disallow it. */
    ctx.mlock_ok = mlockall(MCL_CURRENT | MCL_FUTURE) == 0;
    if (!ctx.mlock_ok)
        fprintf(stderr, "[RT] mlockall failed: %s (measurements still available)\n", strerror(errno));
    else
        fprintf(stderr, "[RT] mlockall(MCL_CURRENT|MCL_FUTURE) succeeded\n");
    if (cb_init(&ctx.buffer, SMS_BUFFER_CAPACITY) != 0) {
        fprintf(stderr, "circular buffer allocation failed\n");
        return 1;
    }
    if (logq_init(&ctx.logq) != 0) {
        cb_destroy(&ctx.buffer);
        return 1;
    }
    snprintf(ctx.mq_name, sizeof(ctx.mq_name), "/sms_alarm_%ld", (long)getpid());
    mq_unlink(ctx.mq_name);
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(SmsAlert);
    ctx.alarm_mq = mq_open(ctx.mq_name, O_CREAT | O_EXCL | O_RDWR, 0600, &attr);
    if (ctx.alarm_mq == (mqd_t)-1) {
        perror("mq_open (is CONFIG_POSIX_MQUEUE enabled in the kernel?)");
        logq_destroy(&ctx.logq);
        cb_destroy(&ctx.buffer);
        return 1;
    }
    /* Start the consumer first so producers can immediately enqueue messages. */
    if (start_worker(&logger, logger_thread_main, &ctx) != 0) goto threads_failed;
    stage = 1;
    if (start_worker(&alert, alert_thread_main, &ctx) != 0) goto threads_failed;
    stage = 2;
    if (start_worker(&processor, processor_thread_main, &ctx) != 0) goto threads_failed;
    stage = 3;
    if (start_worker(&sensor, sensor_thread_main, &ctx) != 0) goto threads_failed;
    stage = 4;
    printf("SMS started: interval=%dms duration=%ds device=%s\n",
           ctx.interval_ms, ctx.duration_s, ctx.device_path);
    fflush(stdout);
    clock_gettime(CLOCK_MONOTONIC, &start);
    while (!got_sigint && !atomic_load(&ctx.stop)) {
        clock_gettime(CLOCK_MONOTONIC, &now);
        if (ctx.duration_s > 0 && now.tv_sec - start.tv_sec >= ctx.duration_s)
            break;
        nanosleep(&nap, NULL);
    }
    goto shutdown;

threads_failed:
    fprintf(stderr, "pthread_create failed\n");
    exit_code = 1;
shutdown:
    atomic_store(&ctx.stop, true);
    if (stage == 4)
        pthread_join(sensor, NULL);
    cb_close(&ctx.buffer);
    if (stage >= 3)
        pthread_join(processor, NULL);
    else
        atomic_store(&ctx.processor_done, true);
    if (stage >= 2)
        pthread_join(alert, NULL);
    logq_close(&ctx.logq);
    if (stage >= 1)
        pthread_join(logger, NULL);
    report_jitter(&ctx);
    stats = cb_get_stats(&ctx.buffer);
    printf("SUMMARY samples=%u processed=%u produced=%llu consumed=%llu overflow=%llu ",
           atomic_load(&ctx.samples_collected), atomic_load(&ctx.processed),
           stats.total_produced, stats.total_consumed, stats.overflow_count);
    printf("alerts: TEMP_HIGH=%u HUMID_HIGH=%u TEMP_SPIKE=%u log=%s\n",
           atomic_load(&ctx.temp_high), atomic_load(&ctx.humid_high),
           atomic_load(&ctx.temp_spike), SMS_LOG_PATH);
    mq_close(ctx.alarm_mq);
    mq_unlink(ctx.mq_name);
    logq_destroy(&ctx.logq);
    cb_destroy(&ctx.buffer);
    return exit_code || atomic_load(&ctx.failed) ? 1 : 0;

bad_args:
    usage(argv[0]);
    return 2;
}
