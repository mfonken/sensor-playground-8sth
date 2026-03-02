#ifndef ERROR_H
#define ERROR_H

// Universal status codes
typedef enum {
    STATUS_OK,
    STATUS_FAIL,
    STATUS_ERROR_I2C,
    STATUS_ERROR_TIMEOUT,
    STATUS_ERROR_NULL_PTR,
    STATUS_ERROR_INVALID_DATA,
    STATUS_ERROR_NO_CONNECTION,
    STATUS_ERROR_TX,
    STATUS_ERROR_RX,
} status_t;

#endif