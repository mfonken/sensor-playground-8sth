#ifndef FIXED_TYPE_H
#define FIXED_TYPE_H

// Dependencies
#include <stdint.h>

// Base type
typedef int32_t fixed_t;

// Constants
#define FIXED_SHIFT     16 // (Q16.16)
#define FIXED_SCALE     (1 << FIXED_SHIFT)          // 65536
#define FIXED_ONE       ((fixed_t)FIXED_SCALE)
#define FIXED_MASK      (FIXED_SCALE - 1)           // 0xFFFF
#define ABS(x)          ((x) < 0 ? -(x) : (x))

#define FIXED_FROM_INT(i)       ((fixed_t)((i) << FIXED_SHIFT))
#define FIXED_TO_INT(f)         ((int32_t)((f) >> FIXED_SHIFT))
#define FIXED_FROM_FLOAT(f)     ((fixed_t)((f) * FIXED_SCALE + 0.5f))  // round
#define FIXED_TO_FLOAT(f)       (((float)f) / FIXED_SCALE)
#define FIXED_DEC_TO_INT(f)     ((int32_t)((((uint64_t)(ABS(f) & FIXED_MASK)) * 10000u) >> FIXED_SHIFT))

// Static operators
// NOTE: Only multiply is needed 
static inline fixed_t fixed_mul(fixed_t a, fixed_t b)
{
    int32_t a_hi = a >> 16;
    int32_t a_lo = a & 0xFFFF;
    int32_t b_hi = b >> 16;
    int32_t b_lo = b & 0xFFFF;

    int32_t result =
        (a_hi * b_hi << 16) +
        (a_hi * b_lo) +
        (a_lo * b_hi) +
        ((a_lo * b_lo) >> 16);

    return result;
}

#endif // FIXED_TYPE_H
