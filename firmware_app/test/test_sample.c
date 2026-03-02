#include <unity.h>
#include <stdio.h>

#include "sample.h"

void setUp(void)
{
}

void tearDown(void)
{
    
}

void test_magnitude(void) 
{
    sample_t sample_pos = {
        .index = 0,
        .x = FIXED_FROM_INT(1),
        .y = FIXED_FROM_INT(2),
        .z = FIXED_FROM_INT(3),
        .mag = 0
    };
    sample_pos.mag = sample_calculate_magnitude(&sample_pos);
    float mag_flt = FIXED_TO_FLOAT(sample_pos.mag);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.74165344f, mag_flt);

    sample_t sample_neg = {
        .index = 0,
        .x = FIXED_FROM_INT(-1),
        .y = FIXED_FROM_INT(-2),
        .z = FIXED_FROM_INT(-3),
        .mag = 0
    };
    sample_neg.mag = sample_calculate_magnitude(&sample_neg);
    mag_flt = FIXED_TO_FLOAT(sample_neg.mag);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.74165344f, mag_flt);

    sample_t sample_large = {
        .index = 0,
        .x = FIXED_FROM_INT(5000),
        .y = FIXED_FROM_INT(5000),
        .z = FIXED_FROM_INT(5000),
        .mag = 0
    };
    sample_large.mag = sample_calculate_magnitude(&sample_large);
    mag_flt = FIXED_TO_FLOAT(sample_large.mag);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 8660.254f, mag_flt);
}

void test_serialize(void) 
{
    sample_t sample1 = {
        .index = 0,
        .x = FIXED_FROM_FLOAT(0.1234f),
        .y = FIXED_FROM_INT(1),
        .z = FIXED_FROM_FLOAT(123.456),
        .mag = 0
    };

#define SAMPLE_STR_LEN 100
    char sample_str[SAMPLE_STR_LEN] = {0};

    snprintf(
        sample_str, 
        SAMPLE_STR_LEN, 
        "%hd.%hu",
        FIXED_TO_INT(sample1.x), 
        FIXED_DEC_TO_INT(sample1.x));
    TEST_ASSERT_EQUAL_STRING("0.1233", sample_str);

    snprintf(
        sample_str, 
        SAMPLE_STR_LEN, 
        "%hd.%hu",
        FIXED_TO_INT(sample1.y), 
        FIXED_DEC_TO_INT(sample1.y));
    TEST_ASSERT_EQUAL_STRING("1.0", sample_str);

    snprintf(
        sample_str, 
        SAMPLE_STR_LEN, 
        "%hd.%hu",
        FIXED_TO_INT(sample1.z), 
        FIXED_DEC_TO_INT(sample1.z));
    TEST_ASSERT_EQUAL_STRING("123.4559", sample_str);
}

int main(void) 
{
    UNITY_BEGIN();
    RUN_TEST(test_magnitude);
    RUN_TEST(test_serialize);
    return UNITY_END();
}