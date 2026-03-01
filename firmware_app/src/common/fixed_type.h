#ifndef FIXED_TYPE_H
#define FIXED_TYPE_H

#include <stdint.h>

// ------------------------------------------------------------
// Simplified fixed point type 
// ------------------------------------------------------------

typedef int32_t fixed_t;

#define FIXED_SHIFT     16 // (Q16.16)
#define FIXED_SCALE     (1 << FIXED_SHIFT)          // 65536
#define FIXED_MASK      (FIXED_SCALE - 1)           // 0xFFFF
#define ABS(x)          ((x) < 0 ? -(x) : (x))

#define FIXED_FROM_INT(i)       ((fixed_t)((i) << FIXED_SHIFT))
#define FIXED_TO_INT(f)         ((int32_t)((f) >> FIXED_SHIFT))
#define FIXED_FROM_FLOAT(f)     ((fixed_t)((f) * FIXED_SCALE + 0.5f))  // round
#define FIXED_TO_FLOAT(f)       (((float)f) / FIXED_SCALE)
#define FIXED_DEC_TO_INT(f)     ((int32_t)(((uint32_t)(f) & FIXED_MASK)))

static inline fixed_t fixed_mul(fixed_t a, fixed_t b) {
    return (fixed_t)(((int64_t)a * b + (1 << (FIXED_SHIFT - 1))) >> FIXED_SHIFT);
}

static inline fixed_t fixed_div(fixed_t a, fixed_t b) {
    // sign‑safe: use int64_t to preserve sign and avoid overflow
    int64_t numerator = ((int64_t)a << FIXED_SHIFT);
    return (fixed_t)(numerator / b);
}

#endif // FIXED_TYPE_H
