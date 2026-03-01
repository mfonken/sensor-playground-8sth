// main.c

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "app.h"

// typedef struct queue_item accelerometer_sample_mg_t;



void initialize(void)
{
    state.sample_index = 0;
    queue_init(&state.sample_queue);
    for(uint8_t i = 0; i < 3; i++)
    {
        iir_filter_init(&state.filter[i], FIXED_FROM_FLOAT(IIR_FILTER_ALPHA), IIR_FILTER_MODE);
    }

    // Accelerometer
    bool ret = false;
    status_t status = accelerometer_verify(&ret);
    if (status != STATUS_OK || !ret) return;

    tcp_init(&state.tcp);
}

void timer_isr(void) 
{
    // ACK timer interrupt
    static uint32_t ticks = 0;

    if (ticks++ % TIMER_TICKS_PER_ACCEL_SAMPLE == 0) 
    {
        i2c_mock_step();

        sample_t * psample = queue_get_tail_ptr(&state.sample_queue);
        
        status_t status = accelerometer_read_all(psample);
        if (status != STATUS_OK) return;

        printf("Enqueue sample %d\n", state.sample_index);
        queue_enqueue(&state.sample_queue, *psample);
        printf("Queue count: %d\n", state.sample_queue.count);
        state.sample_index++;
    }
}

// Compute integer square root of a 16‑bit value (0..65535)
static inline int16_t int_sqrt16(uint16_t n) {
    int16_t x = n;
    int16_t c = 0;
    while ((x << c) > 0x1000) { c++; }  // rough scale down
    x >>= c;
    int16_t root = 0;
    for (int16_t bit = 1 << 7; bit != 0; bit >>= 1) {
        int16_t trial = root + bit;
        if (trial * trial <= x) {
            root = trial;
        }
    }
    return root << (c >> 1);  // scale back up
}

fixed_t calculate_sample_magnitude(sample_t * psample)
{
    int32_t x2 = (psample->x >> (FIXED_SHIFT >> 1)) * (psample->x >> (FIXED_SHIFT >> 1));
    int32_t y2 = (psample->y >> (FIXED_SHIFT >> 1)) * (psample->y >> (FIXED_SHIFT >> 1));
    int32_t z2 = (psample->z >> (FIXED_SHIFT >> 1)) * (psample->z >> (FIXED_SHIFT >> 1));

    int32_t sum2 = x2 + y2 + z2;         // now in Q16.0 scale

    // Clamp to 16‑bit to avoid overflow in sqrt
    if (sum2 > 0xFFFF) sum2 = 0xFFFF;
    if (sum2 < 0)      sum2 = 0;

    // Integer sqrt in 16‑bit
    int16_t sqrt_val = int_sqrt16((uint16_t)sum2);

    // Convert to Q16.16: magnitude is sqrt( x² + y² + z² ) in Q16.16
    return FIXED_FROM_INT(sqrt_val);
}

void app_runner(void) 
{
    initialize();

    for(;;)
    {
        sample_t sample;
        sample_t filtered_sample = {0};
        while(state.tcp.connected)
        {
            accelerometer_enable();
            printf("A\n");
            if (queue_dequeue(&state.sample_queue, &sample)
                // && psample != NULL
                )
            {
                printf("Dequeue sample %d\n", sample.index);
                filtered_sample.index = sample.index;
                filtered_sample.x = iir_filter_apply(&state.filter[0], sample.x);
                filtered_sample.y = iir_filter_apply(&state.filter[1], sample.y);
                filtered_sample.z = iir_filter_apply(&state.filter[2], sample.z);
                filtered_sample.mag = calculate_sample_magnitude(&filtered_sample);
                const char * filtered_sample_str = serialize_sample(&filtered_sample);
                tcp_send(&state.tcp, filtered_sample_str, strlen(filtered_sample_str));
            }
            usleep(1000);
        }
    }
    accelerometer_disable();
}

