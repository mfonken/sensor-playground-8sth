#include "iir_filter.h"

/* Clamp alpha to [0, 1] */
static fixed_t clamp_alpha(fixed_t a)
{
    if (a < 0)
        return 0;

    if (a > FIXED_ONE)
        return FIXED_ONE;

    return a;
}


/* ============================
   Initialization
   ============================ */
void iir_filter_init(iir_filter_t *filter,
                     fixed_t alpha,
                     iir_filter_mode_t mode)
{
    filter->alpha  = clamp_alpha(alpha);
    filter->y_prev = 0;
    filter->x_prev = 0;
    filter->mode   = mode;
}

/* ============================
   Low Pass
   y[n] = α x[n] + (1-α) y[n-1]
   ============================ */
fixed_t iir_low_pass(iir_filter_t * filter, fixed_t x)
{
    fixed_t y = fixed_mul(filter->alpha, x) + fixed_mul(FIXED_ONE - filter->alpha, filter->y_prev);
    filter->y_prev = y;

    return y;
}


/* ============================
   High Pass
   y[n] = α ( y[n-1] + x[n] - x[n-1] )
   ============================ */

fixed_t iir_high_pass(iir_filter_t * filter, fixed_t x)
{
    fixed_t y = fixed_mul(filter->alpha, 
                          filter->y_prev + (x - filter->x_prev));

    filter->x_prev = x;
    filter->y_prev = y;

    return y;
}


/* ============================
   Apply
   ============================ */

fixed_t iir_filter_apply(iir_filter_t *filter, fixed_t x)
{
    if (filter->mode == IIR_FILTER_MODE_LOW_PASS)
        return iir_low_pass(filter, x);
    else
        return iir_high_pass(filter, x);
}