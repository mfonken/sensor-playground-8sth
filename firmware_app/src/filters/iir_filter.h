// iir_filter.h

#ifndef IIR_FILTER_H
#define IIR_FILTER_H

// Dependencies
#include "fixed_type.h"

// Types
typedef enum 
{
    IIR_FILTER_MODE_LOW_PASS,
    IIR_FILTER_MODE_HIGH_PASS
} iir_filter_mode_t;

typedef struct
{
    fixed_t alpha;      /* 0 < alpha < 1 (Q16.16) */
    fixed_t y_prev;     /* previous output */
    fixed_t x_prev;     /* previous input (HPF only) */
    iir_filter_mode_t mode;
} iir_filter_t;


// Functions
void iir_filter_init(iir_filter_t * filter, fixed_t alpha, iir_filter_mode_t mode);
fixed_t iir_low_pass(iir_filter_t * filter, fixed_t x);
fixed_t iir_high_pass(iir_filter_t * filter, fixed_t x);
fixed_t iir_filter_apply(iir_filter_t * filter, fixed_t x);

#endif