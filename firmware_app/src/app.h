// app.h

#ifndef APP_H
#define APP_H

// Dependencies
#include "i2c_mock.h"
#include "accelerometer.h"
#include "circular_queue.h"
#include "iir_filter.h"
#include "tcp.h"

// Configuration constants
#define TIMER_RATE_HZ                   400
#define ACCELEROMETER_SAMPLE_RATE_HZ    100
#define TIMER_TICKS_PER_ACCEL_SAMPLE    (TIMER_RATE_HZ / ACCELEROMETER_SAMPLE_RATE_HZ)

#define APP_RATE_HZ                     (ACCELEROMETER_SAMPLE_RATE_HZ * 2)
#define APP_RATE_US                     (1000000 / APP_RATE_HZ)

#define IIR_FILTER_MODE                 IIR_FILTER_MODE_LOW_PASS // Remove for no filtering
#define IIR_FILTER_ALPHA                FIXED_FROM_FLOAT(0.125)

#define TCP_CONNECTION_PORT             12345

// Types
typedef enum 
{
    APP_STATE_INIT,
    APP_STATE_RUNNING,
    APP_STATE_STOPPED
} sensor_playground_app_status_t;

typedef enum 
{
    APP_CMD_START = '1',
    APP_CMD_STOP = '2',
    APP_CMD_STATUS = '3'
} sensor_playground_app_cmd_t;

typedef struct
{
    uint32_t sample_index;     
    uint32_t tmr_ticks;
    uint32_t error_counter;
    sensor_playground_app_status_t run_state;
    queue_t sample_queue;
    iir_filter_t filter[3];
    tcp_state_t tcp;

} sensor_playground_app_state_t;

// Data
static sensor_playground_app_state_t state;


// Functions
void timer_isr(void);
void app_init(void);
void app_do_work(sample_t * pin, sample_t * pout);
void app_run(void);

#endif
