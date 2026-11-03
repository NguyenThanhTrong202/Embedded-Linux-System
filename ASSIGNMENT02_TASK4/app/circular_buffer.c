/* circular_buffer.c: bounded, thread-safe producer/consumer ring buffer.
 * Author: [fill in your name and student ID] | 2026-11-02
 */
#include "circular_buffer.h"
#include <stdlib.h>
#include <string.h>

int cb_init(CircularBuffer *b, size_t capacity)
{
    if (!capacity)
        return -1;
    memset(b, 0, sizeof(*b));
    b->items = calloc(capacity, sizeof(*b->items));
    if (!b->items)
        return -1;
    b->capacity = capacity;
    if (pthread_mutex_init(&b->lock, NULL) != 0) {
        free(b->items);
        return -1;
    }
    if (pthread_cond_init(&b->not_empty, NULL) != 0) {
        pthread_mutex_destroy(&b->lock);
        free(b->items);
        return -1;
    }
    return 0;
}

void cb_close(CircularBuffer *b)
{
    pthread_mutex_lock(&b->lock);
    b->closed = 1;
    pthread_cond_broadcast(&b->not_empty);
    pthread_mutex_unlock(&b->lock);
}

void cb_destroy(CircularBuffer *b)
{
    pthread_cond_destroy(&b->not_empty);
    pthread_mutex_destroy(&b->lock);
    free(b->items);
    b->items = NULL;
}

int cb_push(CircularBuffer *b, SensorData sample)
{
    int ok = 0;
    pthread_mutex_lock(&b->lock);
    if (!b->closed && b->size < b->capacity) {
        b->items[b->tail] = sample;
        b->tail = (b->tail + 1) % b->capacity;
        b->size++;
        b->total_produced++;
        pthread_cond_signal(&b->not_empty);
        ok = 1;
    } else if (!b->closed) {
        b->overflow_count++;
    }
    pthread_mutex_unlock(&b->lock);
    return ok;
}

int cb_pop(CircularBuffer *b, SensorData *sample)
{
    pthread_mutex_lock(&b->lock);
    while (b->size == 0 && !b->closed)
        pthread_cond_wait(&b->not_empty, &b->lock);
    if (b->size == 0) {
        pthread_mutex_unlock(&b->lock);
        return 0;
    }
    *sample = b->items[b->head];
    b->head = (b->head + 1) % b->capacity;
    b->size--;
    b->total_consumed++;
    pthread_mutex_unlock(&b->lock);
    return 1;
}

BufferStats cb_get_stats(CircularBuffer *b)
{
    BufferStats s;
    pthread_mutex_lock(&b->lock);
    s.current_size = b->size;
    s.total_produced = b->total_produced;
    s.total_consumed = b->total_consumed;
    s.overflow_count = b->overflow_count;
    pthread_mutex_unlock(&b->lock);
    return s;
}
