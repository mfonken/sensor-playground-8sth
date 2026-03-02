// main.c

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "app.h"


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

        // printf("Enqueue sample %d\n", state.sample_index);
        queue_enqueue(&state.sample_queue, *psample);
        // printf("Queue count: %d\n", state.sample_queue.count);
        state.sample_index++;
    }
}

void app_init(void)
{
    // State
    state.sample_index = 0;
    queue_init(&state.sample_queue);

    // Filters
    for(uint8_t i = 0; i < 3; i++)
        iir_filter_init(&state.filter[i], FIXED_FROM_FLOAT(IIR_FILTER_ALPHA), IIR_FILTER_MODE);

    // Accelerometer
    bool ret = false;
    accelerometer_enable();
    status_t status = accelerometer_verify(&ret);
    if (status != STATUS_OK || !ret) return;

    // TCP
    state.tcp.sock_port = TCP_CONNECTION_PORT;
    tcp_init(&state.tcp);
}

void app_do_work(sample_t * pin, sample_t * pout)
{
    // Filter and calculate magnitude
    pout->index = pin->index;
    pout->x = iir_filter_apply(&state.filter[0], pin->x);
    pout->y = iir_filter_apply(&state.filter[1], pin->y);
    pout->z = iir_filter_apply(&state.filter[2], pin->z);
    pout->mag = sample_calculate_magnitude(pout);
}

void app_run(void) 
{
    sample_t sample;
    sample_t filtered_sample = {0};
    uint32_t sample_str_l = 0;
    const char * sample_str;
    tcp_manage_connection(&state.tcp);
    if(state.tcp.connected)
    {
        if (queue_dequeue(&state.sample_queue, &sample))
        {
            app_do_work(&sample, &filtered_sample);
            sample_str = sample_serialize(&filtered_sample, &sample_str_l);
            tcp_send(&state.tcp, sample_str, sample_str_l);
        }
    }
    usleep(APP_RATE_US);
}

