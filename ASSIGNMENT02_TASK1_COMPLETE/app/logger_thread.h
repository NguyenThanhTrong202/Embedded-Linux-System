#ifndef LOGGER_THREAD_H
#define LOGGER_THREAD_H
#include <pthread.h>
#include <stddef.h>
#define LOGQ_CAPACITY 256
#define LOG_LINE_SIZE 256

typedef struct {
    char lines[LOGQ_CAPACITY][LOG_LINE_SIZE];
    size_t head, tail, size;
    unsigned long long dropped;
    int closed;
    pthread_mutex_t lock;
    pthread_cond_t ready;
} LogQueue;

int logq_init(LogQueue *q);
void logq_close(LogQueue *q);
void logq_destroy(LogQueue *q);
int logq_push(LogQueue *q, const char *line);
void *logger_thread_main(void *ctx);
#endif
