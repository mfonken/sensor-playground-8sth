// sample.h

#ifndef SAMPLE_H
#define SAMPLE_H

#include <stdio.h>
#include <stdint.h> 
#include "fixed_type.h"

#define SAMPLE_STR_LEN 100

typedef struct sample
{
    uint32_t index;
    fixed_t x;
    fixed_t y;
    fixed_t z;
    fixed_t mag;
} sample_t;

static char sample_str[SAMPLE_STR_LEN] = {0};

static char * serialize_sample(sample_t * psample)
{
    snprintf(
        sample_str, 
        SAMPLE_STR_LEN, 
        "{\"index\":%u, \"x\":%hd.%hu, \"y\":%hd.%hu, \"z\":%hd.%hu, \"mag\":%hu.%hu}", 
        psample->index, 
        FIXED_TO_INT(psample->x), 
        FIXED_DEC_TO_INT(psample->x),
        
        FIXED_TO_INT(psample->y), 
        FIXED_DEC_TO_INT(psample->y),
        
        FIXED_TO_INT(psample->z), 
        FIXED_DEC_TO_INT(psample->z),
        
        FIXED_TO_INT(psample->mag), 
        FIXED_DEC_TO_INT(psample->mag));
    return sample_str;
}
#endif
