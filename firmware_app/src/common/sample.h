// sample.h

#ifndef SAMPLE_H
#define SAMPLE_H

// Dependencies
#include <stdio.h>
#include <stdint.h> 

#include "fixed_type.h"

// Constants
#define SAMPLE_TX_STR_LEN 100
#define MAG_MAX_COMPONENT  26754

// Types
typedef struct sample
{
    uint32_t index;
    fixed_t x;
    fixed_t y;
    fixed_t z;
    fixed_t mag;
} sample_t;

// Data
static char sample_tx_str[SAMPLE_TX_STR_LEN] = {0};

// Functions
static char * sample_serialize(sample_t * psample, uint32_t * plen)
{
    *plen = snprintf(
        sample_tx_str, 
        SAMPLE_TX_STR_LEN, 
        "{\"index\":%u, \"x\":%hd.%hu, \"y\":%hd.%hu, \"z\":%hd.%hu, \"mag\":%hu.%hu}\n", 
        psample->index, 
        FIXED_TO_INT(psample->x), 
        FIXED_DEC_TO_INT(psample->x),
        
        FIXED_TO_INT(psample->y), 
        FIXED_DEC_TO_INT(psample->y),
        
        FIXED_TO_INT(psample->z), 
        FIXED_DEC_TO_INT(psample->z),
        
        FIXED_TO_INT(psample->mag), 
        FIXED_DEC_TO_INT(psample->mag));
    
    return sample_tx_str;
}

static int32_t isqrt32(int32_t val)
{
    if (val <= 0) return 0;
    if (val == 1) return 1;

    /* Bit-scan initial guess: always >= true sqrt, guarantees
     * monotonic convergence — no oscillation possible */
    int32_t guess = 1;
    int32_t tmp   = val;
    while (tmp > 0) { tmp >>= 2; guess <<= 1; }

    int32_t prev;
    do {
        prev  = guess;
        guess = (guess + val / guess) >> 1;
    } while (guess < prev);

    return prev;
}


static fixed_t sample_calculate_magnitude(sample_t *psample)
{
    int32_t ax = ABS(psample->x);
    int32_t ay = ABS(psample->y);
    int32_t az = ABS(psample->z);

    /* Find largest component */
    int32_t max_val = ax;
    if (ay > max_val) max_val = ay;
    if (az > max_val) max_val = az;

    if (max_val == 0) return 0;

    /* Shift all components right until max fits within MAG_MAX_COMPONENT.
     * This normalizes to use maximum precision for the given input range
     * while guaranteeing 3 * component^2 stays within int32_t. */
    int32_t shift = 0;
    while ((max_val >> shift) > MAG_MAX_COMPONENT) shift++;

    int32_t xs  = ax >> shift;
    int32_t ys  = ay >> shift;
    int32_t zs  = az >> shift;
    int32_t sum = xs*xs + ys*ys + zs*zs;

    /* isqrt operates in the same Q16.16 units as the shifted inputs,
     * so shifting result back up restores the correct Q16.16 magnitude */
    return (fixed_t)(isqrt32(sum) << shift);
}

#endif
