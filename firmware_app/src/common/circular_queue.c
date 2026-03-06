// circular_queue.c

#include "circular_queue.h"
#include <stddef.h>
#include <string.h>

void queue_init(queue_t * queue)
{
    queue->capacity = QUEUE_SIZE;
    queue->head     = 0;
    queue->tail     = 0;
    queue->count    = 0;
    memset(queue->buffer, 0, sizeof(queue->buffer));
}

sample_t * queue_get_tail_ptr(queue_t * queue)
{
    if (queue == NULL) return NULL;
    return &queue->buffer[queue->tail];
}

void queue_enqueue(queue_t * queue, sample_t item)
{
    if (queue == NULL) return;
    queue->buffer[queue->tail] = item;
    queue->tail = (queue->tail + 1) % queue->capacity;



    if (queue->count < queue->capacity)
    {
        queue->count++;
    }
    else
    {
        queue->head = (queue->head + 1) % queue->capacity;
    }
}

bool queue_dequeue(queue_t * queue, sample_t * item)
{
    if (queue == NULL 
        || queue->count == 0)
    {
        return false;
    }
    *item = queue->buffer[queue->head];
    queue->head = (queue->head + 1) % queue->capacity;
    return true;
}