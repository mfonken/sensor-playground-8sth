// accelerometer.c

#include <math.h>

#include "accelerometer.h"
#include "i2c_mock.h"

// Data Handling
int16_t accelerometer_parse_value(uint8_t l_byte, uint8_t h_byte)
{
    int16_t value = 0;
#if ACCEL_VALUE_ENDIAN == 0 // Little endian
    value = ((int16_t)h_byte << 8) | (int16_t)l_byte;
#elif ACCEL_VALUE_ENDIAN == 1 // Big endian
    value = ((int16_t)l_byte << 8) | (int16_t)h_byte;
#else
    #error "ACCEL_VALUE_ENDIAN must be 0 or 1"
#endif

    value = value & 0x0FFF;
    if (value & 0x1000)
    {
        value |= 0xF000;
    }
    return value;
}

fixed_t accelerometer_convert_sample_to_mg(int16_t value)
{
    fixed_t mg = FIXED_FROM_INT(value) * ACCEL_S_FIXED_MG_PER_LSB;
    return mg;
}

fixed_t accelerometer_sample_magnitude(accelerometer_sample_t sample)
{
    int32_t xx = (int32_t)sample.x * sample.x;
    int32_t yy = (int32_t)sample.y * sample.y;
    int32_t zz = (int32_t)sample.z * sample.z;
    int32_t mag = sqrt(xx + yy + zz);

    return accelerometer_convert_sample_to_mg(mag);
}


// Register Functions
status_t accelerometer_write_reg(uint8_t reg, uint8_t value)
{
    status_t status = STATUS_ERROR_I2C;
    if (!ACCEL_VALID_REG(reg))
    {
        status = STATUS_ERROR_INVALID_DATA;
    }
    else
    {
        for(uint8_t i = 0; i < ACCEL_COMM_RETRY; i++)
        {
            if (i2c_write_reg(ACCEL_I2C_ADDRESS, reg, &value, 1) == 0)
            {
                status = STATUS_OK;
                break;
            }
        }
    }
    return status;
}

status_t accelerometer_read_reg(uint8_t reg, uint8_t * pvalue)
{
    status_t status = STATUS_ERROR_I2C;
    if (!ACCEL_VALID_REG(reg))
    {
        status = STATUS_ERROR_INVALID_DATA;
    }
    uint8_t value;
    for(uint8_t i = 0; i < ACCEL_COMM_RETRY; i++)
    {
        if (i2c_read_reg(ACCEL_I2C_ADDRESS, reg, pvalue, 1) == 0)
        {
            status = STATUS_OK;
            break;
        }
    }
    return status;
}

status_t accelerometer_read_config(accelerometer_config_t * pconfig)
{
    status_t status = STATUS_OK;
    if (pconfig == NULL)
    {
        status = STATUS_ERROR_NULL_PTR;
    }
    else
    {
        accelerometer_read_reg(ACCEL_REG_CTRL, (uint8_t *)pconfig);
    }
    return status;
}

// Control Functions
status_t accelerometer_verify(bool * pvalue)
{
    uint8_t value = 0;
    status_t status = accelerometer_read_reg(ACCEL_REG_WHO_AM_I, &value);
    if (status == STATUS_OK)
    {
        *pvalue = value == ACCEL_I_AM;
    }
    return status;
}

status_t accelerometer_is_enabled(bool * pvalue)
{
    accelerometer_config_t config;
    status_t status = accelerometer_read_config(&config);
    if(status == STATUS_OK)
    {
        *pvalue = config.enabled == 1;
    }
    return status;
}

status_t accelerometer_is_hpf_enabled(bool * pvalue)
{
    accelerometer_config_t config;
    status_t status = accelerometer_read_config(&config);
    if(status == STATUS_OK)
    {
        *pvalue = config.hpf_en == 1;
    }
    return status;
}

status_t accelerometer_enable(void)
{
    accelerometer_config_t config;
    status_t status = accelerometer_read_config(&config);
    if(status == STATUS_OK)
    {
        config.enabled = 1;
        status = accelerometer_write_reg(ACCEL_REG_CTRL, *(uint8_t *)&config);
    }
    return status;
}

status_t accelerometer_disable(void)
{
    accelerometer_config_t config;
    status_t status = accelerometer_read_config(&config);
    if(status == STATUS_OK)
    {
        config.enabled = 0;
        status = accelerometer_write_reg(ACCEL_REG_CTRL, *(uint8_t *)&config);
    }
    return status;
}

// Data Functions
status_t accelerometer_read_axis_raw(accelerometer_axis_t axis, int16_t * pvalue)
{
    status_t status = STATUS_OK;
    uint8_t l_val = 0;
    uint8_t h_val = 0;

    uint8_t l_reg = 0;
    uint8_t h_reg = 0;
    switch (axis)
    {
    case ACCEL_X_AXIS:
        l_reg = ACCEL_REG_OUT_X_L;
        h_reg = ACCEL_REG_OUT_X_H;
        break;
    case ACCEL_Y_AXIS:
        l_reg = ACCEL_REG_OUT_Y_L;
        h_reg = ACCEL_REG_OUT_Y_H;
        break;
    case ACCEL_Z_AXIS:
        l_reg = ACCEL_REG_OUT_Z_L;
        h_reg = ACCEL_REG_OUT_Z_H;
        break;
    default:
        status = STATUS_ERROR_INVALID_DATA;
        break;
    }
    if (status != STATUS_OK) return status;

    status = i2c_read_reg(ACCEL_I2C_ADDRESS, l_reg, &l_val, 1);
    if (status != STATUS_OK) return status;

    status = i2c_read_reg(ACCEL_I2C_ADDRESS, h_reg, &h_val, 1);
    if (status != STATUS_OK) return status;
    
    *pvalue = accelerometer_parse_value(l_val, h_val);
    return status;
}

status_t accelerometer_read_axis(accelerometer_axis_t axis, fixed_t * pvalue)
{
    int16_t val;
    status_t status = accelerometer_read_axis_raw(axis, &val);
    if (status == STATUS_OK)
    {
        *pvalue = accelerometer_convert_sample_to_mg(val);
    }
    return status;
}

status_t accelerometer_read_all(accelerometer_sample_mg_t * psample)
{
    status_t status;
    accelerometer_sample_t raw_sample;
    status = accelerometer_read_axis_raw(ACCEL_X_AXIS, &raw_sample.x);
    if (status != STATUS_OK) return status;
    status = accelerometer_read_axis_raw(ACCEL_Y_AXIS, &raw_sample.y);
    if (status != STATUS_OK) return status;
    status = accelerometer_read_axis_raw(ACCEL_Z_AXIS, &raw_sample.z);
    if (status != STATUS_OK) return status;

    psample->x = accelerometer_convert_sample_to_mg(raw_sample.x);
    psample->y = accelerometer_convert_sample_to_mg(raw_sample.y);
    psample->z = accelerometer_convert_sample_to_mg(raw_sample.z);
    psample->mag = accelerometer_sample_magnitude(raw_sample);
    return status;

}


// Mock Functions
void accelerometer_mock_step(void)
{
    i2c_mock_step();
}