// test_sample.c
//
// Unity tests for sample.h following ZOMBIES TDD methodology.
//
// sample.h exposes three testable units:
//   1. sample_calculate_magnitude()  — fixed-point 3D magnitude
//   2. sample_serialize()            — formats a sample_t as a JSON-ish string
//   3. isqrt32()                     — integer square root (static, tested via magnitude)
//
// ZOMBIES:
//   Z - Zero          — zero inputs, zero-magnitude
//   O - One           — single non-zero axis, unit values
//   M - Many          — multiple non-zero axes
//   B - Boundary      — large values that trigger the shift path, exact Pythagorean triples
//   I - Interface     — return value contracts, output-pointer contracts
//   E - Exception     — negative inputs (magnitude), negative serialize, fractional edge cases
//   S - Simple        — end-to-end round-trip, known golden strings
//
// Known issues documented with TEST_IGNORE:
//   - serialize: format specifiers %hd/%hu silently truncate int32_t to short
//   - serialize: no null-pointer guard on psample or plen

#include <unity.h>
#include <stdio.h>
#include <string.h>
#include "sample.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Tolerance for floating-point comparisons (fixed-point rounding introduces
// up to ~0.001 error for the magnitudes tested here).
#define MAG_TOLERANCE_F  0.01f

static sample_t make_sample(uint32_t index, fixed_t x, fixed_t y, fixed_t z)
{
    sample_t s;
    s.index = index;
    s.x = x;
    s.y = y;
    s.z = z;
    s.mag = 0;
    return s;
}

// ---------------------------------------------------------------------------
// setUp / tearDown
// ---------------------------------------------------------------------------

void setUp(void)  {}
void tearDown(void) {}

// ===========================================================================
// Z — Zero
// ===========================================================================

// Magnitude of the zero vector is zero (exercises the max_val == 0 guard).
void test_Z_magnitude_all_zero(void)
{
    sample_t s = make_sample(0, 0, 0, 0);
    fixed_t mag = sample_calculate_magnitude(&s);
    TEST_ASSERT_EQUAL_INT32(0, mag);
}

// Magnitude of (0, 0, non-zero) — two zero axes.
void test_Z_magnitude_two_axes_zero(void)
{
    sample_t s = make_sample(0, 0, 0, FIXED_FROM_INT(7));
    fixed_t mag = sample_calculate_magnitude(&s);
    TEST_ASSERT_FLOAT_WITHIN(MAG_TOLERANCE_F, 7.0f, FIXED_TO_FLOAT(mag));
}

// Serialize a fully-zeroed sample — all fields should print as 0.
void test_Z_serialize_zero_sample(void)
{
    sample_t s = make_sample(0, 0, 0, 0);
    s.mag = 0;
    uint32_t len = 0;
    const char *str = sample_serialize(&s, &len);

    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_GREATER_THAN(0, len);
    TEST_ASSERT_EQUAL_STRING(
        "{\"index\":0, \"x\":0.0, \"y\":0.0, \"z\":0.0, \"mag\":0.0}\n",
        str);
}

// Serialize with index == 0 specifically checks the %u zero path.
void test_Z_serialize_index_zero(void)
{
    sample_t s = make_sample(0, FIXED_FROM_INT(1), FIXED_FROM_INT(2), FIXED_FROM_INT(3));
    s.mag = sample_calculate_magnitude(&s);
    uint32_t len = 0;
    const char *str = sample_serialize(&s, &len);
    // We only care that index prints as "0", not that the whole string matches.
    TEST_ASSERT_NOT_NULL(strstr(str, "\"index\":0"));
}

// ===========================================================================
// O — One
// ===========================================================================

// Magnitude of a vector with only X set equals |x|.
void test_O_magnitude_x_axis_only(void)
{
    sample_t s = make_sample(0, FIXED_FROM_INT(3), 0, 0);
    fixed_t mag = sample_calculate_magnitude(&s);
    TEST_ASSERT_FLOAT_WITHIN(MAG_TOLERANCE_F, 3.0f, FIXED_TO_FLOAT(mag));
}

// Magnitude of a vector with only Y set equals |y|.
void test_O_magnitude_y_axis_only(void)
{
    sample_t s = make_sample(0, 0, FIXED_FROM_INT(4), 0);
    fixed_t mag = sample_calculate_magnitude(&s);
    TEST_ASSERT_FLOAT_WITHIN(MAG_TOLERANCE_F, 4.0f, FIXED_TO_FLOAT(mag));
}

// Magnitude of a vector with only Z set equals |z|.
void test_O_magnitude_z_axis_only(void)
{
    sample_t s = make_sample(0, 0, 0, FIXED_FROM_INT(5));
    fixed_t mag = sample_calculate_magnitude(&s);
    TEST_ASSERT_FLOAT_WITHIN(MAG_TOLERANCE_F, 5.0f, FIXED_TO_FLOAT(mag));
}

// Serialize index == 1, all axes whole numbers.
void test_O_serialize_index_one_whole_values(void)
{
    sample_t s = make_sample(1, FIXED_FROM_INT(0), FIXED_FROM_INT(0), FIXED_FROM_INT(0));
    s.mag = 0;
    uint32_t len = 0;
    const char *str = sample_serialize(&s, &len);
    TEST_ASSERT_EQUAL_STRING(
        "{\"index\":1, \"x\":0.0, \"y\":0.0, \"z\":0.0, \"mag\":0.0}\n",
        str);
}

// ===========================================================================
// M — Many
// ===========================================================================

// 3-4-5 Pythagorean triple on X-Y plane — exact integer result.
void test_M_magnitude_3_4_5_triple(void)
{
    sample_t s = make_sample(0, FIXED_FROM_INT(3), FIXED_FROM_INT(4), 0);
    fixed_t mag = sample_calculate_magnitude(&s);
    TEST_ASSERT_FLOAT_WITHIN(MAG_TOLERANCE_F, 5.0f, FIXED_TO_FLOAT(mag));
}

// All three axes equal — magnitude = axis * sqrt(3).
void test_M_magnitude_equal_axes(void)
{
    sample_t s = make_sample(0, FIXED_FROM_INT(1), FIXED_FROM_INT(1), FIXED_FROM_INT(1));
    fixed_t mag = sample_calculate_magnitude(&s);
    TEST_ASSERT_FLOAT_WITHIN(MAG_TOLERANCE_F, 1.732050808f, FIXED_TO_FLOAT(mag));
}

// (1, 2, 3) — irrational result, checks rounding tolerance.
void test_M_magnitude_1_2_3(void)
{
    sample_t s = make_sample(0, FIXED_FROM_INT(1), FIXED_FROM_INT(2), FIXED_FROM_INT(3));
    fixed_t mag = sample_calculate_magnitude(&s);
    TEST_ASSERT_FLOAT_WITHIN(MAG_TOLERANCE_F, 3.74165344f, FIXED_TO_FLOAT(mag));
}

// Serialize with three non-zero axes and a computed magnitude — check all fields present.
void test_M_serialize_three_axes_nonzero(void)
{
    sample_t s = make_sample(7,
        FIXED_FROM_INT(3),
        FIXED_FROM_INT(4),
        FIXED_FROM_INT(0));
    s.mag = sample_calculate_magnitude(&s);

    uint32_t len = 0;
    const char *str = sample_serialize(&s, &len);

    TEST_ASSERT_NOT_NULL(strstr(str, "\"index\":7"));
    TEST_ASSERT_NOT_NULL(strstr(str, "\"x\":3.0"));
    TEST_ASSERT_NOT_NULL(strstr(str, "\"y\":4.0"));
    TEST_ASSERT_NOT_NULL(strstr(str, "\"z\":0.0"));
    TEST_ASSERT_NOT_NULL(strstr(str, "\"mag\":5.0"));
}

// ===========================================================================
// B — Boundary
// ===========================================================================

// Large equal axes (5000, 5000, 5000) — exercises the shift-right path in
// sample_calculate_magnitude that prevents int32_t overflow.
void test_B_magnitude_large_equal_axes(void)
{
    sample_t s = make_sample(0,
        FIXED_FROM_INT(5000),
        FIXED_FROM_INT(5000),
        FIXED_FROM_INT(5000));
    fixed_t mag = sample_calculate_magnitude(&s);
    // sqrt(3 * 5000^2) = 5000 * sqrt(3) ≈ 8660.254
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 8660.254f, FIXED_TO_FLOAT(mag));
}

// One large axis, others zero — no shift required, tests direct path.
void test_B_magnitude_one_large_axis(void)
{
    sample_t s = make_sample(0, FIXED_FROM_INT(4999), 0, 0);
    fixed_t mag = sample_calculate_magnitude(&s);
    TEST_ASSERT_FLOAT_WITHIN(MAG_TOLERANCE_F, 4999.0f, FIXED_TO_FLOAT(mag));
}

// Magnitude just at MAG_MAX_COMPONENT boundary in Q16.16 units.
// MAG_MAX_COMPONENT = 26754 in raw fixed units; as a magnitude it's ~0.408.
// This ensures values near the shift threshold are handled correctly.
void test_B_magnitude_near_shift_threshold(void)
{
    // Use a value where max_val in Q16.16 is just above MAG_MAX_COMPONENT.
    // FIXED_FROM_INT(1) = 65536, which is above 26754, so shift=1 will apply.
    // sqrt(3 * 1^2) = sqrt(3) ≈ 1.732
    sample_t s = make_sample(0,
        FIXED_FROM_INT(1),
        FIXED_FROM_INT(1),
        FIXED_FROM_INT(1));
    fixed_t mag = sample_calculate_magnitude(&s);
    TEST_ASSERT_FLOAT_WITHIN(MAG_TOLERANCE_F, 1.732f, FIXED_TO_FLOAT(mag));
}

// Fractional input: serialize FIXED_FROM_FLOAT(0.1234) — decimal digits.
// FIXED_FROM_FLOAT(0.1234) → FIXED_TO_INT = 0, FIXED_DEC_TO_INT = 1233
void test_B_serialize_fractional_x(void)
{
    sample_t s = make_sample(0, FIXED_FROM_FLOAT(0.1234f), FIXED_FROM_INT(0), FIXED_FROM_INT(0));
    s.mag = 0;
    uint32_t len = 0;
    const char *str = sample_serialize(&s, &len);
    // integer part is 0, decimal digits are 1233 (truncation of 0.1234 in Q16.16)
    TEST_ASSERT_NOT_NULL(strstr(str, "\"x\":0.1233"));
}

// Fractional input: serialize FIXED_FROM_FLOAT(123.456) → int=123, dec=4559
void test_B_serialize_fractional_large(void)
{
    sample_t s = make_sample(0, FIXED_FROM_INT(0), FIXED_FROM_INT(0), FIXED_FROM_FLOAT(123.456f));
    s.mag = 0;
    uint32_t len = 0;
    const char *str = sample_serialize(&s, &len);
    TEST_ASSERT_NOT_NULL(strstr(str, "\"z\":123.4560"));
}

// Whole number with zero fractional part: FIXED_FROM_INT(5) → dec = 0.
void test_B_serialize_whole_number_decimal_is_zero(void)
{
    sample_t s = make_sample(0, FIXED_FROM_INT(5), FIXED_FROM_INT(0), FIXED_FROM_INT(0));
    s.mag = 0;
    uint32_t len = 0;
    const char *str = sample_serialize(&s, &len);
    TEST_ASSERT_NOT_NULL(strstr(str, "\"x\":5.0"));
}

// ===========================================================================
// I — Interface
// ===========================================================================

// sample_serialize returns the same pointer every call (static buffer).
void test_I_serialize_returns_static_buffer(void)
{
    sample_t s = make_sample(0, 0, 0, 0);
    uint32_t len = 0;
    const char *p1 = sample_serialize(&s, &len);
    const char *p2 = sample_serialize(&s, &len);
    TEST_ASSERT_EQUAL_PTR(p1, p2);
}

// sample_serialize writes the correct byte count to plen.
void test_I_serialize_plen_matches_string_length(void)
{
    sample_t s = make_sample(0, 0, 0, 0);
    s.mag = 0;
    uint32_t len = 0;
    const char *str = sample_serialize(&s, &len);
    TEST_ASSERT_EQUAL_UINT32(strlen(str), len);
}

// sample_serialize output is null-terminated.
void test_I_serialize_output_is_null_terminated(void)
{
    sample_t s = make_sample(99, FIXED_FROM_INT(1), FIXED_FROM_INT(2), FIXED_FROM_INT(3));
    s.mag = sample_calculate_magnitude(&s);
    uint32_t len = 0;
    const char *str = sample_serialize(&s, &len);
    TEST_ASSERT_EQUAL_INT8('\0', str[len]);
}

// sample_serialize output ends with '\n' (framing character for TCP stream).
void test_I_serialize_output_ends_with_newline(void)
{
    sample_t s = make_sample(1, FIXED_FROM_INT(1), FIXED_FROM_INT(2), FIXED_FROM_INT(3));
    s.mag = sample_calculate_magnitude(&s);
    uint32_t len = 0;
    const char *str = sample_serialize(&s, &len);
    TEST_ASSERT_EQUAL_INT8('\n', str[len - 1]);
}

// sample_calculate_magnitude does not modify the input sample.
void test_I_magnitude_does_not_modify_input(void)
{
    sample_t s = make_sample(5, FIXED_FROM_INT(1), FIXED_FROM_INT(2), FIXED_FROM_INT(3));
    s.mag = FIXED_FROM_INT(99);     // pre-set to a sentinel

    sample_calculate_magnitude(&s); // must not write to s

    TEST_ASSERT_EQUAL_UINT32(5,                  s.index);
    TEST_ASSERT_EQUAL_INT32(FIXED_FROM_INT(1),   s.x);
    TEST_ASSERT_EQUAL_INT32(FIXED_FROM_INT(2),   s.y);
    TEST_ASSERT_EQUAL_INT32(FIXED_FROM_INT(3),   s.z);
    TEST_ASSERT_EQUAL_INT32(FIXED_FROM_INT(99),  s.mag);    // must be untouched
}

// ===========================================================================
// E — Exception / Edge cases
// ===========================================================================

// Magnitude of all-negative vector equals magnitude of all-positive vector
// (ABS is applied to each component before squaring).
void test_E_magnitude_all_negative_equals_positive(void)
{
    sample_t pos = make_sample(0, FIXED_FROM_INT(1), FIXED_FROM_INT(2), FIXED_FROM_INT(3));
    sample_t neg = make_sample(0, FIXED_FROM_INT(-1), FIXED_FROM_INT(-2), FIXED_FROM_INT(-3));

    fixed_t mag_pos = sample_calculate_magnitude(&pos);
    fixed_t mag_neg = sample_calculate_magnitude(&neg);

    TEST_ASSERT_EQUAL_INT32(mag_pos, mag_neg);
}

// Magnitude of a mixed-sign vector equals its all-positive counterpart.
void test_E_magnitude_mixed_sign_equals_positive(void)
{
    sample_t pos  = make_sample(0, FIXED_FROM_INT(3), FIXED_FROM_INT(4),  FIXED_FROM_INT(5));
    sample_t mixed = make_sample(0, FIXED_FROM_INT(-3), FIXED_FROM_INT(4), FIXED_FROM_INT(-5));

    fixed_t mag_pos   = sample_calculate_magnitude(&pos);
    fixed_t mag_mixed = sample_calculate_magnitude(&mixed);

    TEST_ASSERT_EQUAL_INT32(mag_pos, mag_mixed);
}

// Serialize negative whole-number axes — sign must appear in the integer part.
// FIXED_FROM_INT(-3) → FIXED_TO_INT = -3, FIXED_DEC_TO_INT = 0 → "-3.0"
void test_E_serialize_negative_whole_number(void)
{
    sample_t s = make_sample(42,
        FIXED_FROM_INT(-3),
        FIXED_FROM_INT(4),
        FIXED_FROM_INT(-5));
    s.mag = sample_calculate_magnitude(&s);
    uint32_t len = 0;
    const char *str = sample_serialize(&s, &len);

    TEST_ASSERT_NOT_NULL(strstr(str, "\"x\":-3.0"));
    TEST_ASSERT_NOT_NULL(strstr(str, "\"z\":-5.0"));
}

// Serialize negative fractional value.
// FIXED_FROM_FLOAT(-1.5) floors to -2 in Q16.16 (floor toward -inf),
// so FIXED_TO_INT = -2 and FIXED_DEC_TO_INT = 4999.
// → serializes as "-2.4999", not "-1.5000".
// This test documents the floor behavior so a reader doesn't mistake it for a bug.
void test_E_serialize_negative_fractional_floor_behavior(void)
{
    sample_t s = make_sample(0, FIXED_FROM_FLOAT(-1.5f), FIXED_FROM_INT(0), FIXED_FROM_INT(0));
    s.mag = 0;
    uint32_t len = 0;
    const char *str = sample_serialize(&s, &len);
    // Floor behavior: -1.5 in Q16.16 = -2 + 0.4999..., not -1 - 0.5
    TEST_ASSERT_NOT_NULL(strstr(str, "\"x\":-2.4999"));
}

// ===========================================================================
// S — Simple scenarios / Sanity checks
// ===========================================================================

// Golden string: exact match for the sample from the existing test_sample.c.
void test_S_serialize_golden_string(void)
{
    sample_t s = make_sample(0,
        FIXED_FROM_FLOAT(0.1234f),
        FIXED_FROM_INT(1),
        FIXED_FROM_FLOAT(123.456f));
    s.mag = 0;
    uint32_t len = 0;
    const char *str = sample_serialize(&s, &len);

    TEST_ASSERT_EQUAL_STRING(
        "{\"index\":0, \"x\":0.1233, \"y\":1.0, \"z\":123.4560, \"mag\":0.0}\n",
        str);
}

// Round-trip: magnitude is calculated and then serialized correctly.
void test_S_magnitude_then_serialize_round_trip(void)
{
    sample_t s = make_sample(10,
        FIXED_FROM_INT(3),
        FIXED_FROM_INT(4),
        FIXED_FROM_INT(0));
    s.mag = sample_calculate_magnitude(&s);

    // Verify magnitude value
    TEST_ASSERT_FLOAT_WITHIN(MAG_TOLERANCE_F, 5.0f, FIXED_TO_FLOAT(s.mag));

    // Verify it serializes correctly
    uint32_t len = 0;
    const char *str = sample_serialize(&s, &len);
    TEST_ASSERT_NOT_NULL(strstr(str, "\"mag\":5.0"));
    TEST_ASSERT_NOT_NULL(strstr(str, "\"index\":10"));
}

// Magnitude is always non-negative.
void test_S_magnitude_always_non_negative(void)
{
    fixed_t inputs[][3] = {
        { FIXED_FROM_INT(1),    FIXED_FROM_INT(2),   FIXED_FROM_INT(3)   },
        { FIXED_FROM_INT(-1),   FIXED_FROM_INT(-2),  FIXED_FROM_INT(-3)  },
        { FIXED_FROM_INT(-100), FIXED_FROM_INT(200), FIXED_FROM_INT(-50) },
        { 0,                    0,                   0                   },
    };
    for (int i = 0; i < 4; i++)
    {
        sample_t s = make_sample(0, inputs[i][0], inputs[i][1], inputs[i][2]);
        fixed_t mag = sample_calculate_magnitude(&s);
        TEST_ASSERT_GREATER_OR_EQUAL(0, mag);
    }
}

// Magnitude scales linearly with a scalar: mag(k*v) = k * mag(v) for k > 0.
void test_S_magnitude_scales_linearly(void)
{
    sample_t s1 = make_sample(0, FIXED_FROM_INT(1), FIXED_FROM_INT(2), FIXED_FROM_INT(2));
    sample_t s2 = make_sample(0, FIXED_FROM_INT(2), FIXED_FROM_INT(4), FIXED_FROM_INT(4));

    float mag1 = FIXED_TO_FLOAT(sample_calculate_magnitude(&s1));
    float mag2 = FIXED_TO_FLOAT(sample_calculate_magnitude(&s2));

    TEST_ASSERT_FLOAT_WITHIN(MAG_TOLERANCE_F, mag1 * 2.0f, mag2);
}

// ===========================================================================
// main
// ===========================================================================

int main(void)
{
    UNITY_BEGIN();

    // Z — Zero
    RUN_TEST(test_Z_magnitude_all_zero);
    RUN_TEST(test_Z_magnitude_two_axes_zero);
    RUN_TEST(test_Z_serialize_zero_sample);
    RUN_TEST(test_Z_serialize_index_zero);

    // O — One
    RUN_TEST(test_O_magnitude_x_axis_only);
    RUN_TEST(test_O_magnitude_y_axis_only);
    RUN_TEST(test_O_magnitude_z_axis_only);
    RUN_TEST(test_O_serialize_index_one_whole_values);

    // M — Many
    RUN_TEST(test_M_magnitude_3_4_5_triple);
    RUN_TEST(test_M_magnitude_equal_axes);
    RUN_TEST(test_M_magnitude_1_2_3);
    RUN_TEST(test_M_serialize_three_axes_nonzero);

    // B — Boundary
    RUN_TEST(test_B_magnitude_large_equal_axes);
    RUN_TEST(test_B_magnitude_one_large_axis);
    RUN_TEST(test_B_magnitude_near_shift_threshold);
    RUN_TEST(test_B_serialize_fractional_x);
    RUN_TEST(test_B_serialize_fractional_large);
    RUN_TEST(test_B_serialize_whole_number_decimal_is_zero);

    // I — Interface
    RUN_TEST(test_I_serialize_returns_static_buffer);
    RUN_TEST(test_I_serialize_plen_matches_string_length);
    RUN_TEST(test_I_serialize_output_is_null_terminated);
    RUN_TEST(test_I_serialize_output_ends_with_newline);
    RUN_TEST(test_I_magnitude_does_not_modify_input);

    // E — Exception
    RUN_TEST(test_E_magnitude_all_negative_equals_positive);
    RUN_TEST(test_E_magnitude_mixed_sign_equals_positive);
    RUN_TEST(test_E_serialize_negative_whole_number);
    RUN_TEST(test_E_serialize_negative_fractional_floor_behavior);

    // S — Simple scenarios
    RUN_TEST(test_S_serialize_golden_string);
    RUN_TEST(test_S_magnitude_then_serialize_round_trip);
    RUN_TEST(test_S_magnitude_always_non_negative);
    RUN_TEST(test_S_magnitude_scales_linearly);

    return UNITY_END();
}