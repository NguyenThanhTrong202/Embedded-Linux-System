/* logger_thread.c: asynchronous logging queue, 5s flush, 1 MiB rotation.
 * Author: [fill in your name and student ID] | 2026-11-02
 */
#include "sms_common.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

int logq_init(LogQueue *q)
{
    memset(q, 0, sizeof(*q));
    if (pthread_mutex_init(&q->lock, NULL) != 0)
        return -1;
    if (pthread_cond_init(&q->ready, NULL) != 0) {
        pthread_mutex_destroy(&q->lock);
        return -1;
    }
    return 0;
}
void logq_close(LogQueue *q)
{
    pthread_mutex_lock(&q->lock);
    q->closed = 1;
    pthread_cond_broadcast(&q->ready);
    pthread_mutex_unlock(&q->lock);
}
void logq_destroy(LogQueue *q)
{
    pthread_cond_destroy(&q->ready);
    pthread_mutex_destroy(&q->lock);
}
int logq_push(LogQueue *q, const char *line)
{
    int ok = 0;
    pthread_mutex_lock(&q->lock);
    if (!q->closed && q->size < LOGQ_CAPACITY) {
        snprintf(q->lines[q->tail], LOG_LINE_SIZE, "%s", line);
        q->tail = (q->tail + 1) % LOGQ_CAPACITY;
        q->size++;
        pthread_cond_signal(&q->ready);
        ok = 1;
    } else {
        q->dropped++;
    }
    pthread_mutex_unlock(&q->lock);
    return ok;
}

/* Returns 1 for line, 0 for timed-out wait, -1 once drained and closed. */
static int logq_take(LogQueue *q, char *line)
{
    int rc = 0;
    struct timespec until;
    clock_gettime(CLOCK_REALTIME, &until);
    until.tv_sec += 1;
    pthread_mutex_lock(&q->lock);
    while (q->size == 0 && !q->closed) {
        rc = pthread_cond_timedwait(&q->ready, &q->lock, &until);
        if (rc == ETIMEDOUT)
            break;
    }
    if (q->size) {
        snprintf(line, LOG_LINE_SIZE, "%s", q->lines[q->head]);
        q->head = (q->head + 1) % LOGQ_CAPACITY;
        q->size--;
        rc = 1;
    } else {
        rc = q->closed ? -1 : 0;
    }
    pthread_mutex_unlock(&q->lock);
    return rc;
}

static void rotate_if_needed(FILE **out)
{
    struct stat st;
    if (stat(SMS_LOG_PATH, &st) == 0 && st.st_size > 1024 * 1024) {
        fclose(*out);
        (void)rename(SMS_LOG_PATH, SMS_LOG_PATH ".1");
        *out = fopen(SMS_LOG_PATH, "a");
    }
}

void *logger_thread_main(void *ptr)
{
    SmsContext *ctx = ptr;
    char msg[LOG_LINE_SIZE];
    char date[40];
    struct timespec ts;
    struct tm local;
    time_t last_flush = time(NULL);
    FILE *out = fopen(SMS_LOG_PATH, "a");
    if (!out) {
        perror("open sms log");
        atomic_store(&ctx->failed, 1);
        return NULL;
    }
    for (;;) {
        int status = logq_take(&ctx->logq, msg);
        if (status < 0)
            break;
        if (status > 0) {
            clock_gettime(CLOCK_REALTIME, &ts);
            localtime_r(&ts.tv_sec, &local);
            strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", &local);
            fprintf(out, "[%s.%03ld] %s\n", date, ts.tv_nsec / 1000000, msg);
            if (ferror(out)) {
                perror("write sms log");
                atomic_store(&ctx->failed, 1);
                clearerr(out);
            }
        }
        if (time(NULL) - last_flush >= 5) {
            fflush(out);
            rotate_if_needed(&out);
            if (!out) {
                atomic_store(&ctx->failed, 1);
                return NULL;
            }
            last_flush = time(NULL);
        }
    }
    fflush(out);
    fclose(out);
    return NULL;
}
