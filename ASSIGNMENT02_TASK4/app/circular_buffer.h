#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    unsigned long long timestamp_ms;
    double temperature;
    double humidity;
} SensorData;

typedef struct {
    SensorData *items;
    size_t capacity, head, tail, size;
    unsigned long long total_produced, total_consumed, overflow_count;
    int closed;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
} CircularBuffer;

typedef struct {
    size_t current_size;
    unsigned long long total_produced, total_consumed, overflow_count;
} BufferStats;

int cb_init(CircularBuffer *b, size_t capacity);
void cb_close(CircularBuffer *b);
void cb_destroy(CircularBuffer *b);
int cb_push(CircularBuffer *b, SensorData sample); /* 1=ok, 0=full */
int cb_pop(CircularBuffer *b, SensorData *sample); /* 1=ok, 0=closed/empty */
BufferStats cb_get_stats(CircularBuffer *b);
#endif
