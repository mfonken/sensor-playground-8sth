// app.h

#ifndef APP_H
#define APP_H

#include "i2c_mock.h"
#include "accelerometer.h"
#include "circular_queue.h"
#include "iir_filter.h"
#include "tcp_mock.h"

#define TIMER_RATE_HZ                   400
#define ACCELEROMETER_SAMPLE_RATE_HZ    100
#define TIMER_TICKS_PER_ACCEL_SAMPLE    (TIMER_RATE_HZ / ACCELEROMETER_SAMPLE_RATE_HZ)

#define APP_RATE_HZ                     (ACCELEROMETER_SAMPLE_RATE_HZ * 2)
#define APP_RATE_US                     (1000000 / APP_RATE_HZ)

#define IIR_FILTER_MODE                 IIR_FILTER_MODE_LOW_PASS
#define IIR_FILTER_ALPHA                (1/8)

#define TCP_CONNECTION_PORT             12345

typedef struct
{
    uint32_t sample_index;
    queue_t sample_queue;
    iir_filter_t filter[3];
    tcp_state_t tcp;
} sensor_playground_app_state_t;
static sensor_playground_app_state_t state;

void timer_isr(void);
void app_init(void);
void app_run(void);

#endif
