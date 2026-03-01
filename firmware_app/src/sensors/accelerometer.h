// accelerometer.h

#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H

#include "error.h"
#include "fixed_type.h"

#include <stdint.h>
#include <stdbool.h>

#define ACCEL_REG_WHO_AM_I  0x00
#define ACCEL_REG_CTRL      0x01
#define ACCEL_REG_OUT_X_L   0x10
#define ACCEL_REG_OUT_X_H   0x11
#define ACCEL_REG_OUT_Y_L   0x12
#define ACCEL_REG_OUT_Y_H   0x13
#define ACCEL_REG_OUT_Z_L   0x14
#define ACCEL_REG_OUT_Z_H   0x15

#define ACCEL_VALID_REG(R)  ((R) == ACCEL_REG_WHO_AM_I || (R) == ACCEL_REG_CTRL || ((R) >= ACCEL_REG_OUT_X_L && (R) <= ACCEL_REG_OUT_Z_H))
#define ACCEL_ERROR         0xFF
#define ACCEL_COMM_RETRY    ((uint8_t)3)

#define ACCEL_I2C_ADDRESS   0x1A
#define ACCEL_I_AM          0x42

#define ACCEL_LSB_RES       12
#define ACCEL_RANGE_MG      5000 // +/- 5000mg
#define ACCEL_S_FIXED_MG_PER_LSB (FIXED_FROM_FLOAT((float)ACCEL_RANGE_MG / (float)(1 << ACCEL_LSB_RES)))

#define ACCEL_VALUE_ENDIAN  0 // 0 = little, 1 = big

// Data types
typedef struct
{
    uint8_t enabled: 1;
    uint8_t hpf_en: 1;
} accelerometer_config_t;

typedef enum
{
    ACCEL_X_AXIS = 0,
    ACCEL_Y_AXIS = 1,
    ACCEL_Z_AXIS = 2
} accelerometer_axis_t;

typedef enum
{
    ACCEL_VAL_ERROR     = 0,
    ACCEL_VAL_CONVERTED = 2,
    ACCEL_VAL_LPF       = 2,
    ACCEL_VAL_HPF       = 3,
    ACCEL_VAL_LPF_HPF   = 4
} accelerometer_value_info_t;

typedef struct 
{
    int16_t x;
    int16_t y;
    int16_t z;
} accelerometer_sample_t;

typedef struct 
{
    fixed_t x;
    fixed_t y;
    fixed_t z;
    fixed_t mag;
    accelerometer_value_info_t info;
} accelerometer_sample_mg_t;

// Data Handling
int16_t accelerometer_parse_value(uint8_t l_byte, uint8_t h_byte);
fixed_t accelerometer_convert_sample_to_mg(int16_t value);
fixed_t accelerometer_sample_magnitude(accelerometer_sample_t value);

// Register Functions
status_t accelerometer_write_reg(uint8_t reg, uint8_t value);
status_t accelerometer_read_reg(uint8_t reg, uint8_t * pvalue);
status_t accelerometer_read_config(accelerometer_config_t * pconfig);

// Control Functions
status_t accelerometer_verify(bool * pvalue);
status_t accelerometer_is_enabled(bool * pvalue);
status_t accelerometer_is_hpf_enabled(bool * pvalue);
status_t accelerometer_enable(void);
status_t accelerometer_disable(void);

// Data Functions
status_t accelerometer_read_axis_raw(accelerometer_axis_t axis, int16_t * pvalue);
status_t accelerometer_read_axis(accelerometer_axis_t axis, fixed_t * pvalue);
status_t accelerometer_read_all(accelerometer_sample_mg_t * psample);

// Mock Functions
void accelerometer_mock_step(void);

#endif