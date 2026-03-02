// test_accelerometer.c
// Unity tests for accelerometer module following ZOMBIES TDD methodology.
//
// ZOMBIES:
//   Z - Zero
//   O - One
//   M - Many
//   B - Boundary
//   I - Interface
//   E - Exception / Error
//   S - Simple scenarios / Sanity checks

#include "unity.h"
#include "accelerometer.h"
#include "i2c_mock.h"
#include "sample.h"
#include "fixed_type.h"

// ---------------------------------------------------------------------------
// setUp / tearDown
// ---------------------------------------------------------------------------

void setUp(void)
{
    // Reset mock to a known, enabled state before each test
    i2c_mock_set_xyz(0.0f, 0.0f, 0.0f);
    accelerometer_enable();
}

void tearDown(void)
{
    // Nothing needed — mock state resets in setUp
}

// ===========================================================================
// Z — Zero
// Inputs of zero, empty / uninitialised state.
// ===========================================================================

// Zero: parse_value with both bytes zero → raw value is 0
void test_parse_value_zero_bytes(void)
{
    int16_t result = accelerometer_parse_value(0x00, 0x00);
    TEST_ASSERT_EQUAL_INT16(0, result);
}

// Zero: convert raw 0 to mg → 0
void test_convert_sample_to_mg_zero(void)
{
    fixed_t result = accelerometer_convert_sample_to_mg(0);
    TEST_ASSERT_EQUAL_INT32(0, result);
}

// Zero: read_all when sensor reports 0,0,0
void test_read_all_zero_signal(void)
{
    i2c_mock_set_xyz(0.0f, 0.0f, 0.0f);
    i2c_mock_step();

    sample_t sample = {0};
    status_t status = accelerometer_read_all(&sample);

    TEST_ASSERT_EQUAL(STATUS_OK, status);
    TEST_ASSERT_EQUAL_INT32(0, sample.x);
    TEST_ASSERT_EQUAL_INT32(0, sample.y);
    TEST_ASSERT_EQUAL_INT32(0, sample.z);
}

// ===========================================================================
// O — One
// Single, simplest meaningful input.
// ===========================================================================

// One: verify returns true when WHO_AM_I register matches ACCEL_I_AM
void test_verify_returns_true_for_valid_device(void)
{
    bool result = false;
    status_t status = accelerometer_verify(&result);

    TEST_ASSERT_EQUAL(STATUS_OK, status);
    TEST_ASSERT_TRUE(result);
}

// One: enable sets enabled bit in CTRL register
void test_enable_sets_enabled_flag(void)
{
    accelerometer_disable();      // ensure it starts disabled
    status_t status = accelerometer_enable();
    TEST_ASSERT_EQUAL(STATUS_OK, status);

    bool enabled = false;
    accelerometer_is_enabled(&enabled);
    TEST_ASSERT_TRUE(enabled);
}

// One: disable clears enabled bit
void test_disable_clears_enabled_flag(void)
{
    accelerometer_enable();
    status_t status = accelerometer_disable();
    TEST_ASSERT_EQUAL(STATUS_OK, status);

    bool enabled = true;
    accelerometer_is_enabled(&enabled);
    TEST_ASSERT_FALSE(enabled);
}

// One: read a single axis — X
void test_read_axis_x_one_value(void)
{
    i2c_mock_set_xyz(100.0f, 0.0f, 0.0f);
    i2c_mock_step();

    fixed_t value = 0;
    status_t status = accelerometer_read_axis(ACCEL_X_AXIS, &value);

    TEST_ASSERT_EQUAL(STATUS_OK, status);
    TEST_ASSERT_NOT_EQUAL(0, value);   // non-zero x was injected
}

// ===========================================================================
// M — Many
// Multiple inputs, repeated operations.
// ===========================================================================

// Many: read_all returns correct values for all three axes simultaneously
void test_read_all_three_axes_nonzero(void)
{
    i2c_mock_set_xyz(100.0f, 200.0f, 300.0f);
    i2c_mock_step();

    sample_t sample = {0};
    status_t status = accelerometer_read_all(&sample);

    TEST_ASSERT_EQUAL(STATUS_OK, status);
    TEST_ASSERT_NOT_EQUAL(0, sample.x);
    TEST_ASSERT_NOT_EQUAL(0, sample.y);
    TEST_ASSERT_NOT_EQUAL(0, sample.z);
}

// Many: multiple sequential reads return independent, up-to-date values
void test_sequential_reads_reflect_new_values(void)
{
    i2c_mock_set_xyz(50.0f, 50.0f, 50.0f);
    i2c_mock_step();
    sample_t first = {0};
    accelerometer_read_all(&first);

    i2c_mock_set_xyz(500.0f, 500.0f, 500.0f);
    i2c_mock_step();
    sample_t second = {0};
    accelerometer_read_all(&second);

    // Second read should have larger magnitude than first
    TEST_ASSERT_GREATER_THAN(first.x, second.x);
}

// Many: convert_sample_to_mg is consistent across several known values
void test_convert_sample_to_mg_multiple_values(void)
{
    // Positive values should increase monotonically
    fixed_t v1 = accelerometer_convert_sample_to_mg(100);
    fixed_t v2 = accelerometer_convert_sample_to_mg(200);
    fixed_t v3 = accelerometer_convert_sample_to_mg(400);

    TEST_ASSERT_GREATER_THAN(0, v1);
    TEST_ASSERT_GREATER_THAN(v1, v2);
    TEST_ASSERT_GREATER_THAN(v2, v3);
}

// ===========================================================================
// B — Boundary
// Min/max values, edge of valid ranges.
// ===========================================================================

// Boundary: parse_value with maximum positive 12-bit value (0x7FF = 2047)
void test_parse_value_max_positive(void)
{
    // 12-bit max positive: 0x07FF → l_byte=0xFF, h_byte=0x07
    int16_t result = accelerometer_parse_value(0xFF, 0x07);
    TEST_ASSERT_EQUAL_INT16(2047, result);
}

// Boundary: parse_value with minimum negative 12-bit value (0x800 = -2048)
void test_parse_value_min_negative(void)
{
    // 12-bit min negative: 0x0800 → l_byte=0x00, h_byte=0x08
    int16_t result = accelerometer_parse_value(0x00, 0x08);
    TEST_ASSERT_EQUAL_INT16(-2048, result);
}

// Boundary: parse_value ignores bits above bit 11 (upper nibble of h_byte)
void test_parse_value_masks_upper_bits(void)
{
    // Same lower 12 bits, garbage in the upper nibble
    int16_t clean  = accelerometer_parse_value(0xFF, 0x07);
    int16_t noisy  = accelerometer_parse_value(0xFF, 0xF7);  // upper nibble 0xF
    TEST_ASSERT_EQUAL_INT16(clean, noisy);
}

// Boundary: convert_sample_to_mg with max positive raw (2047)
void test_convert_sample_to_mg_max_positive(void)
{
    fixed_t result = accelerometer_convert_sample_to_mg(2047);
    // Should be close to +ACCEL_RANGE_MG (5000 mg) but just under
    int32_t mg = FIXED_TO_INT(result);
    TEST_ASSERT_GREATER_THAN(0, mg);
    TEST_ASSERT_LESS_OR_EQUAL(5000, mg);
}

// Boundary: convert_sample_to_mg with max negative raw (-2048)
void test_convert_sample_to_mg_max_negative(void)
{
    fixed_t result = accelerometer_convert_sample_to_mg(-2048);
    int32_t mg = FIXED_TO_INT(result);
    TEST_ASSERT_LESS_THAN(0, mg);
    TEST_ASSERT_GREATER_OR_EQUAL(-5000, mg);
}

// ===========================================================================
// I — Interface
// Public API contracts, return types, output-pointer conventions.
// ===========================================================================

// Interface: accelerometer_verify writes through its output pointer
void test_verify_writes_output_pointer(void)
{
    bool result = false;           // initialised to wrong value
    status_t status = accelerometer_verify(&result);

    TEST_ASSERT_EQUAL(STATUS_OK, status);
    // Mock always responds with ACCEL_I_AM, so result must be true
    TEST_ASSERT_TRUE(result);
}

// Interface: accelerometer_read_axis writes through its output pointer
void test_read_axis_writes_output_pointer(void)
{
    i2c_mock_set_xyz(123.0f, 0.0f, 0.0f);
    i2c_mock_step();

    fixed_t value = 0xDEADBEEF;   // sentinel
    status_t status = accelerometer_read_axis(ACCEL_X_AXIS, &value);

    TEST_ASSERT_EQUAL(STATUS_OK, status);
    TEST_ASSERT_NOT_EQUAL((fixed_t)0xDEADBEEF, value);  // must have been overwritten
}

// Interface: read_all fills all three fields of sample_t
void test_read_all_fills_all_sample_fields(void)
{
    i2c_mock_set_xyz(10.0f, 20.0f, 30.0f);
    i2c_mock_step();

    sample_t sample;
    sample.x = 0xDEAD;
    sample.y = 0xDEAD;
    sample.z = 0xDEAD;

    status_t status = accelerometer_read_all(&sample);

    TEST_ASSERT_EQUAL(STATUS_OK, status);
    TEST_ASSERT_NOT_EQUAL((fixed_t)0xDEAD, sample.x);
    TEST_ASSERT_NOT_EQUAL((fixed_t)0xDEAD, sample.y);
    TEST_ASSERT_NOT_EQUAL((fixed_t)0xDEAD, sample.z);
}

// Interface: all axis enum values are accepted by read_axis
void test_read_axis_accepts_all_valid_axis_values(void)
{
    i2c_mock_set_xyz(1.0f, 2.0f, 3.0f);
    i2c_mock_step();

    fixed_t val;
    TEST_ASSERT_EQUAL(STATUS_OK, accelerometer_read_axis(ACCEL_X_AXIS, &val));
    TEST_ASSERT_EQUAL(STATUS_OK, accelerometer_read_axis(ACCEL_Y_AXIS, &val));
    TEST_ASSERT_EQUAL(STATUS_OK, accelerometer_read_axis(ACCEL_Z_AXIS, &val));
}

// ===========================================================================
// E — Exception / Error
// Invalid inputs, null pointers, bad register addresses, I2C failures.
// ===========================================================================

// Exception: accelerometer_read_config with NULL pointer → STATUS_ERROR_NULL_PTR
void test_read_config_null_ptr_returns_error(void)
{
    status_t status = accelerometer_read_config(NULL);
    TEST_ASSERT_EQUAL(STATUS_ERROR_NULL_PTR, status);
}

// Exception: accelerometer_verify with NULL pointer → STATUS_ERROR_NULL_PTR
void test_verify_null_ptr_does_not_crash(void)
{
    status_t status = accelerometer_verify(NULL);
    TEST_ASSERT_EQUAL(STATUS_ERROR_NULL_PTR, status);
}

// Exception: read_axis with invalid axis value → STATUS_ERROR_INVALID_DATA
void test_read_axis_invalid_axis_returns_error(void)
{
    fixed_t val = 0;
    status_t status = accelerometer_read_axis_raw((accelerometer_axis_t)99, (int16_t *)&val);
    TEST_ASSERT_EQUAL(STATUS_ERROR_INVALID_DATA, status);
}

// Exception: write_reg with invalid register → STATUS_ERROR_INVALID_DATA
void test_write_reg_invalid_register_returns_error(void)
{
    // 0x99 is not in the valid register set
    status_t status = accelerometer_write_reg(0x99, 0x00);
    TEST_ASSERT_EQUAL(STATUS_ERROR_INVALID_DATA, status);
}

// Exception: read_reg with invalid register → STATUS_ERROR_INVALID_DATA
void test_read_reg_invalid_register_returns_error(void)
{
    uint8_t val = 0;
    status_t status = accelerometer_read_reg(0x99, &val);
    TEST_ASSERT_EQUAL(STATUS_ERROR_INVALID_DATA, status);
}

// ===========================================================================
// S — Simple scenarios / Sanity checks
// End-to-end, behavioural "does the whole thing make sense" tests.
// ===========================================================================

// Sanity: x-axis value sign matches injected sign (positive input → positive mg)
void test_positive_x_input_yields_positive_mg(void)
{
    i2c_mock_set_xyz(500.0f, 0.0f, 0.0f);
    i2c_mock_step();

    sample_t sample = {0};
    accelerometer_read_all(&sample);

    TEST_ASSERT_GREATER_THAN(0, sample.x);
}

// Sanity: negative x input → negative mg output
void test_negative_x_input_yields_negative_mg(void)
{
    i2c_mock_set_xyz(-500.0f, 0.0f, 0.0f);
    i2c_mock_step();

    sample_t sample = {0};
    accelerometer_read_all(&sample);

    TEST_ASSERT_LESS_THAN(0, sample.x);
}

// Sanity: enable → disable → is_enabled roundtrip
void test_enable_disable_roundtrip(void)
{
    accelerometer_enable();
    bool en = false;
    accelerometer_is_enabled(&en);
    TEST_ASSERT_TRUE(en);

    accelerometer_disable();
    accelerometer_is_enabled(&en);
    TEST_ASSERT_FALSE(en);

    accelerometer_enable();
    accelerometer_is_enabled(&en);
    TEST_ASSERT_TRUE(en);
}

// Sanity: larger injected signal produces proportionally larger mg reading
void test_larger_signal_produces_larger_reading(void)
{
    i2c_mock_set_xyz(100.0f, 0.0f, 0.0f);
    i2c_mock_step();
    sample_t small_sample = {0};
    accelerometer_read_all(&small_sample);

    i2c_mock_set_xyz(1000.0f, 0.0f, 0.0f);
    i2c_mock_step();
    sample_t large_sample = {0};
    accelerometer_read_all(&large_sample);

    TEST_ASSERT_GREATER_THAN(small_sample.x, large_sample.x);
}

// Sanity: convert_sample_to_mg is antisymmetric (negating input negates output)
void test_convert_sample_to_mg_is_antisymmetric(void)
{
    int16_t raw = 512;
    fixed_t pos = accelerometer_convert_sample_to_mg(raw);
    fixed_t neg = accelerometer_convert_sample_to_mg(-raw);

    // |pos| == |neg| and opposite sign
    TEST_ASSERT_EQUAL_INT32(pos, -neg);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(void)
{
    UNITY_BEGIN();

    // Z — Zero
    RUN_TEST(test_parse_value_zero_bytes);
    RUN_TEST(test_convert_sample_to_mg_zero);
    RUN_TEST(test_read_all_zero_signal);

    // O — One
    RUN_TEST(test_verify_returns_true_for_valid_device);
    RUN_TEST(test_enable_sets_enabled_flag);
    RUN_TEST(test_disable_clears_enabled_flag);
    RUN_TEST(test_read_axis_x_one_value);

    // M — Many
    RUN_TEST(test_read_all_three_axes_nonzero);
    RUN_TEST(test_sequential_reads_reflect_new_values);
    RUN_TEST(test_convert_sample_to_mg_multiple_values);

    // B — Boundary
    RUN_TEST(test_parse_value_max_positive);
    RUN_TEST(test_parse_value_min_negative);
    RUN_TEST(test_parse_value_masks_upper_bits);
    RUN_TEST(test_convert_sample_to_mg_max_positive);
    RUN_TEST(test_convert_sample_to_mg_max_negative);

    // I — Interface
    RUN_TEST(test_verify_writes_output_pointer);
    RUN_TEST(test_read_axis_writes_output_pointer);
    RUN_TEST(test_read_all_fills_all_sample_fields);
    RUN_TEST(test_read_axis_accepts_all_valid_axis_values);

    // E — Exception / Error
    RUN_TEST(test_read_config_null_ptr_returns_error);
    RUN_TEST(test_verify_null_ptr_does_not_crash);
    RUN_TEST(test_read_axis_invalid_axis_returns_error);
    RUN_TEST(test_write_reg_invalid_register_returns_error);
    RUN_TEST(test_read_reg_invalid_register_returns_error);

    // S — Simple scenarios / Sanity
    RUN_TEST(test_positive_x_input_yields_positive_mg);
    RUN_TEST(test_negative_x_input_yields_negative_mg);
    RUN_TEST(test_enable_disable_roundtrip);
    RUN_TEST(test_larger_signal_produces_larger_reading);
    RUN_TEST(test_convert_sample_to_mg_is_antisymmetric);

    return UNITY_END();
}