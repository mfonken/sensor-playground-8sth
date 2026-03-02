#ifndef TCP_H
#define TCP_H

#include <stdint.h>
#include <stdbool.h>

#include "error.h"

// Types
typedef struct {
    int sock_port;
    int sock_fd;
    int client_fd;
    volatile int connected;
    volatile int initialized;
    volatile int running;
} tcp_state_t;

// Functions
void tcp_manage_connection(tcp_state_t *state);
status_t tcp_init(tcp_state_t *state);
status_t tcp_send(tcp_state_t *state, const char *sample, int len);
status_t tcp_receive(tcp_state_t *state, char *buf, int len);
bool tcp_is_connected(tcp_state_t *state);
void tcp_close(tcp_state_t *state);

#endif
