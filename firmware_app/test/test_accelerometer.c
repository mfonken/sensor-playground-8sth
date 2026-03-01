#include <unity.h>

#include "accelerometer.h"

void setUp(void)
{

}

void tearDown(void)
{
    
}

void test_scale_calc(void) 
{
    TEST_ASSERT_EQUAL(ACCEL_S_FIXED_MG_PER_LSB, 80000);
}

void test_WHO_AM_I_verify(void) 
{
    bool valid;
    status_t status = accelerometer_verify(&valid);
    TEST_ASSERT_EQUAL(status, STATUS_OK);
    TEST_ASSERT_EQUAL(valid, true);
}

void test_enable(void) 
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
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_scale_calc);
    RUN_TEST(test_WHO_AM_I_verify);
    RUN_TEST(test_enable);
    return UNITY_END();
}