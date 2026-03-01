#ifndef ERROR_H
#define ERROR_H

typedef enum {
    STATUS_OK,
    STATUS_ERROR_I2C,
    STATUS_ERROR_TIMEOUT,
    STATUS_ERROR_NULL_PTR,
    STATUS_ERROR_INVALID_DATA,
} status_t;

#endif