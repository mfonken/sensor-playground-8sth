#ifndef TCP_MOCK_H
#define TCP_MOCK_H

#include <stdint.h>
typedef struct {
    int sock_port;
    int sock_fd;
    int client_fd;
    volatile int connected;
    volatile int initialized;
    volatile int running;
} tcp_state_t;

void tcp_manage_connection(tcp_state_t *state);

// Initialize TCP (called automatically first time)
int tcp_init(tcp_state_t *state);

// Send sample (auto-reconnects if needed)
int tcp_send(tcp_state_t *state, const char *sample, int len);

// Returns 1 if connected
int tcp_is_connected(tcp_state_t *state);

// Cleanup
void tcp_close(tcp_state_t *state);

#endif
