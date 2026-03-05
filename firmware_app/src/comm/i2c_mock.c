// i2c_mock.c

#include "i2c_mock.h"
#include <stdlib.h>
#include <math.h>

#include "accelerometer.h"

typedef struct 
{
    uint8_t x_l;
    uint8_t x_h;
    uint8_t y_l;
    uint8_t y_h;
    uint8_t z_l;
    uint8_t z_h;

    int16_t x;
    int16_t y;
    int16_t z;
    int16_t variance;
    int16_t range;
    bool test_signal;

    bool enabled;
    bool hpf_enabled;
} internal_state_t;


static internal_state_t internal_state = 
{
    .x_l = 0,
    .x_h = 0,
    .y_l = 0,
    .y_h = 0,
    .z_l = 0,
    .z_h = 0,

    .x = 0,
    .y = 0,
    .z = 0,
    .variance = 100,
    .range = 5000,
    .test_signal = true,

    .enabled = false,
    .hpf_enabled = false
};

int i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *buf, size_t len)
{
    if (addr != ACCEL_I2C_ADDRESS) return -1;
    switch(reg)
    {
    case ACCEL_REG_WHO_AM_I:
        *buf = ACCEL_I_AM;
        break;
    case ACCEL_REG_CTRL:
        *buf = (internal_state.enabled & 1) | ((internal_state.hpf_enabled & 2) << 1);
        break;
    case ACCEL_REG_OUT_X_L:
        *buf = internal_state.x_l;
        break;
    case ACCEL_REG_OUT_X_H:
        *buf = internal_state.x_h;
        break;
    case ACCEL_REG_OUT_Y_L:
        *buf = internal_state.y_l;
        break;
    case ACCEL_REG_OUT_Y_H:
        *buf = internal_state.y_h;
        break;
    case ACCEL_REG_OUT_Z_L:
        *buf = internal_state.z_l;
        break;
    case ACCEL_REG_OUT_Z_H:
        *buf = internal_state.z_h;
        break;
    default:
        return -1;
    }
    return 0;
}

int i2c_write_reg(uint8_t addr, uint8_t reg, const uint8_t *buf, size_t len)
{
    if (addr != ACCEL_I2C_ADDRESS || buf == NULL || len != 1) return -1;
    switch(reg)
    {
    case ACCEL_REG_CTRL:
        internal_state.enabled = *buf & 1;
        internal_state.hpf_enabled = *buf & (1 << 1);
        return 0;
    default:
        return -1;
    }
}

#include <stdio.h>
// For testing: advances internal "time" and updates sensor data. 
// Not something you would normally have in real firmware, but here to drive the simulation. 
void i2c_mock_step(void)
{
    static int step_count = 0;
    static float amplitude = 1000.0f / ((float)ACCEL_RANGE_MG / (float)(1 << (ACCEL_LSB_RES - 1)));
    static float period = 40.0f;
// #define ACCEL_SPOOF_IN_PHASE
#ifdef ACCEL_SPOOF_IN_PHASE
    static float x_offset = 0.0f;
    static float y_offset = 0.0f; 
    static float z_offset = 0.0f;
#else
    static float x_offset = 0.0f;
    static float y_offset = 0.333f * 2 * M_PI;
    static float z_offset = 0.666f * 2 * M_PI;
#endif

    float x_new = internal_state.x;
    float y_new = internal_state.y;
    float z_new = internal_state.z;
    if (internal_state.test_signal)
    {
        x_new = amplitude * cosf(2 * M_PI * step_count / period + x_offset);
        y_new = amplitude * cosf(2 * M_PI * step_count / period + y_offset);
        z_new = amplitude * cosf(2 * M_PI * step_count / period + z_offset);
        step_count++;
        // x_new = internal_state.x + (float)rand() / RAND_MAX * 2 * internal_state.variance - internal_state.variance;
        // y_new = internal_state.y + (float)rand() / RAND_MAX * 2 * internal_state.variance - internal_state.variance;
        // z_new = internal_state.z + (float)rand() / RAND_MAX * 2 * internal_state.variance - internal_state.variance;
    }
        
    int16_t x_new_int = (int16_t)(x_new);
    internal_state.x_l = (uint8_t)(x_new_int & 0xFF);
    internal_state.x_h = (uint8_t)((x_new_int >> 8) & 0xFF);

    int16_t y_new_int = (int16_t)(y_new);
    internal_state.y_l = (uint8_t)(y_new_int & 0xFF);
    internal_state.y_h = (uint8_t)((y_new_int >> 8) & 0xFF);

    int16_t z_new_int = (int16_t)(z_new);
    internal_state.z_l = (uint8_t)(z_new_int & 0xFF);
    internal_state.z_h = (uint8_t)((z_new_int >> 8) & 0xFF);
}

void i2c_mock_set_xyz(float x, float y, float z)
{
    internal_state.test_signal = false;
    internal_state.x = x;
    internal_state.y = y;
    internal_state.z = z;
}