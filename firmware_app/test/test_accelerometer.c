#include <unity.h>

#include "i2c_mock.h"
#include "accelerometer.h"

void setUp(void)
{

}

void tearDown(void)
{
    
}

void test_scale_calc(void) 
{
    TEST_ASSERT_EQUAL(ACCEL_S_FIXED_MG_PER_LSB, FIXED_FROM_FLOAT(2.44140625));
}

void test_WHO_AM_I_verify(void) 
{
    bool valid;
    status_t status = accelerometer_verify(&valid);
    TEST_ASSERT_EQUAL(status, STATUS_OK);
    TEST_ASSERT_EQUAL(valid, true);
}

void test_accel_control(void) 
{
    bool enabled = false;
    status_t status = accelerometer_is_enabled(&enabled);
    TEST_ASSERT_EQUAL(status, STATUS_OK);
    TEST_ASSERT_EQUAL(enabled, false);

    status = accelerometer_enable();
    TEST_ASSERT_EQUAL(status, STATUS_OK);

    status = accelerometer_is_enabled(&enabled);
    TEST_ASSERT_EQUAL(status, STATUS_OK);
    TEST_ASSERT_EQUAL(enabled, true);

    status = accelerometer_disable();
    TEST_ASSERT_EQUAL(status, STATUS_OK);

    status = accelerometer_is_enabled(&enabled);
    TEST_ASSERT_EQUAL(status, STATUS_OK);
    TEST_ASSERT_EQUAL(enabled, false);

    enabled = false;
    status = accelerometer_is_hpf_enabled(&enabled);
    TEST_ASSERT_EQUAL(status, STATUS_OK);
    TEST_ASSERT_EQUAL(enabled, false);
}

void test_accel_read(void)
{
    status_t status;

    // Read raw
    int16_t raw_value = 0;

    i2c_mock_set_xyz(1, 2, 3);
    i2c_mock_step();
    status = accelerometer_read_axis_raw(ACCEL_X_AXIS, &raw_value);
    TEST_ASSERT_EQUAL(status, STATUS_OK);
    TEST_ASSERT_EQUAL(raw_value, 1);
    status = accelerometer_read_axis_raw(ACCEL_Y_AXIS, &raw_value);
    TEST_ASSERT_EQUAL(status, STATUS_OK);
    TEST_ASSERT_EQUAL(raw_value, 2);
    status = accelerometer_read_axis_raw(ACCEL_Z_AXIS, &raw_value);
    TEST_ASSERT_EQUAL(status, STATUS_OK);
    TEST_ASSERT_EQUAL(raw_value, 3);

    i2c_mock_set_xyz(-2048, 2047, 2048);
    i2c_mock_step();
    status = accelerometer_read_axis_raw(ACCEL_X_AXIS, &raw_value);
    TEST_ASSERT_EQUAL(status, STATUS_OK);
    TEST_ASSERT_EQUAL(raw_value, -2048);
    status = accelerometer_read_axis_raw(ACCEL_Y_AXIS, &raw_value);
    TEST_ASSERT_EQUAL(status, STATUS_OK);
    TEST_ASSERT_EQUAL(raw_value, 2047);
    status = accelerometer_read_axis_raw(ACCEL_Z_AXIS, &raw_value);
    TEST_ASSERT_EQUAL(status, STATUS_OK);
    TEST_ASSERT_NOT_EQUAL(raw_value, 2048);
    
    // Read scaled
    i2c_mock_set_xyz(1, 2, -2);
    i2c_mock_step();
    fixed_t scaled_value = FIXED_FROM_INT(-2);
    TEST_ASSERT_EQUAL(FIXED_TO_FLOAT(scaled_value), -2); // Sanity check

    status = accelerometer_read_axis(ACCEL_X_AXIS, &scaled_value);
    TEST_ASSERT_EQUAL(status, STATUS_OK);
    int scaled_int = FIXED_TO_INT(scaled_value);
    TEST_ASSERT_EQUAL(scaled_int, 2);

    status = accelerometer_read_axis(ACCEL_Y_AXIS, &scaled_value);
    TEST_ASSERT_EQUAL(status, STATUS_OK);
    scaled_int = FIXED_TO_INT(scaled_value);
    TEST_ASSERT_EQUAL(scaled_int, 4);

    status = accelerometer_read_axis(ACCEL_Z_AXIS, &scaled_value);
    TEST_ASSERT_EQUAL(status, STATUS_OK);
    scaled_int = FIXED_TO_INT(scaled_value);
    TEST_ASSERT_EQUAL(scaled_int, -5);

    i2c_mock_set_xyz(-2048, 2047, 2048);
    i2c_mock_step();
    scaled_value = FIXED_FROM_INT(-5000);
    TEST_ASSERT_EQUAL(FIXED_TO_FLOAT(scaled_value), -5000); // Sanity check

    status = accelerometer_read_axis(ACCEL_X_AXIS, &scaled_value);
    TEST_ASSERT_EQUAL(status, STATUS_OK);
    scaled_int = FIXED_TO_INT(scaled_value);
    TEST_ASSERT_EQUAL(scaled_int, -5000);

    status = accelerometer_read_axis(ACCEL_Y_AXIS, &scaled_value);
    TEST_ASSERT_EQUAL(status, STATUS_OK);
    scaled_int = FIXED_TO_INT(scaled_value);
    TEST_ASSERT_EQUAL(scaled_int, 4997);

    status = accelerometer_read_axis(ACCEL_Z_AXIS, &scaled_value);
    TEST_ASSERT_EQUAL(status, STATUS_OK);
    scaled_int = FIXED_TO_INT(scaled_value);
    TEST_ASSERT_NOT_EQUAL(scaled_int, 5001);

    // Read all
    i2c_mock_set_xyz(1, 2, -2);
    i2c_mock_step();
    sample_t sample;
    status = accelerometer_read_all(&sample);
    TEST_ASSERT_EQUAL(status, STATUS_OK);
    int x_int = FIXED_TO_INT(sample.x);
    int y_int = FIXED_TO_INT(sample.y);
    int z_int = FIXED_TO_INT(sample.z);
    TEST_ASSERT_EQUAL(x_int, 2);
    TEST_ASSERT_EQUAL(y_int, 4);
    TEST_ASSERT_EQUAL(z_int, -5);

    i2c_mock_set_xyz(-2048, 2047, 2048);
    i2c_mock_step();
    status = accelerometer_read_all(&sample);
    TEST_ASSERT_EQUAL(status, STATUS_OK);
    x_int = FIXED_TO_INT(sample.x);
    y_int = FIXED_TO_INT(sample.y);
    z_int = FIXED_TO_INT(sample.z);
    TEST_ASSERT_EQUAL(x_int, -5000);
    TEST_ASSERT_EQUAL(y_int, 4997);
    TEST_ASSERT_NOT_EQUAL(z_int, 5001);
}

int main(void) 
{
    UNITY_BEGIN();
    RUN_TEST(test_scale_calc);
    RUN_TEST(test_WHO_AM_I_verify);
    RUN_TEST(test_accel_control);
    RUN_TEST(test_accel_read);
    return UNITY_END();
}