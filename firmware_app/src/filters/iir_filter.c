// iir_filter.c

#include "iir_filter.h"

void iir_filter_init(iir_filter_t * filter, fixed_t alpha, iir_filter_mode_t mode) 
{
    filter->alpha = alpha;
    filter->x = 0;
    filter->x_ = 0;
    filter->mode = mode;
}

fixed_t iir_low_pass(iir_filter_t * filter, fixed_t x) 
{
    filter->x = fixed_mul(x, filter->alpha) + fixed_mul(filter->x_, (1 - filter->alpha));
    return filter->x;
}

fixed_t iir_high_pass(iir_filter_t * filter, fixed_t x) 
{
    filter->x = fixed_mul(filter->x, filter->alpha) + fixed_mul((x - filter->x_), filter->alpha);
    filter->x_ = x;
    return filter->x;
}

fixed_t iir_filter_apply(iir_filter_t * filter, fixed_t x)
{
    if (filter->mode == IIR_FILTER_MODE_LOW_PASS)
        return iir_low_pass(filter, x);
    else
        return iir_high_pass(filter, x);
}