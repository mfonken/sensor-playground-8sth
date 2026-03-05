// main.c

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "app.h"


void timer_isr(void) 
{
    // ACK timer interrupt
    if (state.tmr_ticks++ % TIMER_TICKS_PER_ACCEL_SAMPLE == 0) 
    {
        i2c_mock_step();

        sample_t * psample = queue_get_tail_ptr(&state.sample_queue);
        status_t status = accelerometer_read_all(psample);
        if (status != STATUS_OK) return;

        queue_enqueue(&state.sample_queue, *psample);
        state.sample_index++;
    }
}

static void app_check_rx(void)
{
    char rx = 0;
    uint32_t status_l;
    char * status_str = sample_tx_str; // Use sample's string
    uint32_t status_str_max_len = SAMPLE_TX_STR_LEN;
    status_t rx_status = tcp_receive(&state.tcp, &rx, 1);
    if (rx_status == STATUS_OK) 
    {
        switch(rx)
        {
            case APP_CMD_START: // '1'
                status_str = "~Starting app\n";
                status_l = strlen(status_str);
                printf("%s", status_str);
                state.run_state = APP_STATE_RUNNING;
                tcp_send(&state.tcp, status_str, status_l);
                break;
            case APP_CMD_STOP: // '2'
                status_str = "~Stopping app\n";
                status_l = strlen(status_str);
                printf("%s", status_str);
                state.run_state = APP_STATE_STOPPED;
                tcp_send(&state.tcp, status_str, status_l);
                break;
            case APP_CMD_STATUS: // '3'
                status_l = snprintf(
                    status_str,
                    status_str_max_len, 
                    "{\"sample_count\":%u, \"error_count\":%u}\n", 
                    state.sample_index,        
                    state.error_counter
                );
                printf("Status: %s\n", status_str);
                tcp_send(&state.tcp, status_str, status_l);
                break;
            default:
                break;
        }
    }
}


status_t app_init(void)
{
    // State
    state.run_state = APP_STATE_INIT;
    state.sample_index = 0;
    state.tmr_ticks = 0;
    queue_init(&state.sample_queue);

    // Filters
    for(uint8_t i = 0; i < 3; i++)
        iir_filter_init(&state.filter[i], FIXED_FROM_FLOAT(IIR_FILTER_ALPHA), IIR_FILTER_MODE);

    // Accelerometer
    bool ret = false;
    status_t status = accelerometer_enable();
    if (status == STATUS_OK)
        status = accelerometer_verify(&ret);

    if (status != STATUS_OK)
    {
        state.error_counter++;
        state.run_state = APP_STATE_STOPPED;
        return status;
    }

    // TCP
    state.tcp.sock_port = TCP_CONNECTION_PORT;
    status = tcp_init(&state.tcp);
    if (status != STATUS_OK)
    {
        state.error_counter++;
        state.run_state = APP_STATE_STOPPED;
        return status;
    }
    state.run_state = APP_STATE_RUNNING;
    return STATUS_OK;
}

void app_do_work(sample_t * pin, sample_t * pout)
{
#ifdef IIR_FILTER_MODE
    pout->x = iir_filter_apply(&state.filter[0], pin->x);
    pout->y = iir_filter_apply(&state.filter[1], pin->y);
    pout->z = iir_filter_apply(&state.filter[2], pin->z);
#else
    pout = pin;
#endif
    // Filter and calculate magnitude
    pout->index = pin->index;
    pout->mag = sample_calculate_magnitude(pout);
}

void app_run(void) 
{
    sample_t sample;
    sample_t filtered_sample;
    uint32_t sample_str_l = 0;
    const char * sample_str;
    
    tcp_manage_connection(&state.tcp);
    if(state.tcp.connected)
    {
        // Check for rx from client
        app_check_rx();

        if (queue_dequeue(&state.sample_queue, &sample))
        {
            app_do_work(&sample, &filtered_sample);

            if (state.run_state != APP_STATE_RUNNING) 
                return;
            sample_str = sample_serialize(&filtered_sample, &sample_str_l);
            tcp_send(&state.tcp, sample_str, sample_str_l);
        }
    }
    usleep(APP_RATE_US);
}

