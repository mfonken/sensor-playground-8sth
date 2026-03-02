// circular_queue.h

#ifndef CIRCULAR_QUEUE_H
#define CIRCULAR_QUEUE_H

// Dependencies
#include <stdint.h>
#include <stdbool.h>

#include "sample.h"

// Constants
#define QUEUE_SIZE (uint8_t)32

// Types
typedef struct
{
    sample_t buffer[QUEUE_SIZE];
    uint8_t capacity;
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} queue_t;

// Functions
void queue_init(queue_t * queue);
sample_t * queue_get_tail_ptr(queue_t * queue);
void queue_enqueue(queue_t * queue, sample_t item);
bool queue_dequeue(queue_t * queue, sample_t * item);

#endif 