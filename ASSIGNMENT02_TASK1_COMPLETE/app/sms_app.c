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
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t got_sigint;
static void handle_sigint(int signo) { (void)signo; got_sigint = 1; }

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s [-i ms] [-t Celsius] [-h percent] [-d seconds] [-v] [-p device]\n", prog);
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
    snprintf(ctx.device_path, sizeof(ctx.device_path), "%s", SMS_DEFAULT_DEVICE);
    while ((opt = getopt(argc, argv, "i:t:h:d:vp:")) != -1) {
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
    if (pthread_create(&logger, NULL, logger_thread_main, &ctx) != 0) goto threads_failed;
    stage = 1;
    if (pthread_create(&alert, NULL, alert_thread_main, &ctx) != 0) goto threads_failed;
    stage = 2;
    if (pthread_create(&processor, NULL, processor_thread_main, &ctx) != 0) goto threads_failed;
    stage = 3;
    if (pthread_create(&sensor, NULL, sensor_thread_main, &ctx) != 0) goto threads_failed;
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
