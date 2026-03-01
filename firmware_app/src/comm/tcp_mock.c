#include "tcp_mock.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>

#define PORT 12345

#define MAX_SAMPLE_STRING 100

static void* connection_manager(void *arg) {
    tcp_state_t *state = (tcp_state_t*)arg;
    
    while (state->running) {
        // Listen for new connections
        struct pollfd pfd = { .fd = state->sock_fd, .events = POLLIN };
        if (poll(&pfd, 1, 1000) > 0 && pfd.revents & POLLIN) {
            if (state->client_fd >= 0) close(state->client_fd);
            state->client_fd = accept(state->sock_fd, NULL, NULL);
            if (state->client_fd >= 0) {
                state->connected = 1;
                printf("TCP connected\n");
            }
        }
        
        // Check client liveness
        if (state->client_fd >= 0) {
            struct pollfd cfd = { .fd = state->client_fd, .events = POLLIN };
            if (poll(&cfd, 1, 0) <= 0) {
                close(state->client_fd);
                state->client_fd = -1;
                state->connected = 0;
            }
        }
        
        usleep(100000);  // 100ms poll
    }
    return NULL;
}

int tcp_init(tcp_state_t *state) {
    if (state->initialized) return 0;
    
    // Create socket
    state->sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (state->sock_fd < 0) return -1;
    
    int opt = 1;
    setsockopt(state->sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(state->sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(state->sock_fd);
        return -1;
    }
    
    listen(state->sock_fd, 5);
    state->running = 1;
    state->initialized = 1;
    
    pthread_create(&state->thread, NULL, connection_manager, state);
    printf("TCP server ready on port %d\n", PORT);
    return 0;
}

int tcp_send(tcp_state_t *state, const char *sample, int len) {
    // Auto-init if needed
    if (!state->initialized && tcp_init(state) != 0) return -1;
    
    // Auto-reconnect wait
    int retries = 20;
    while (!state->connected && state->running && retries--) {
        usleep(50000);
    }
    
    if (!state->connected || state->client_fd < 0) {
        printf("No connection\n");
        return -1;
    }
    
    int bytes = send(state->client_fd, sample, len, 0);
    return (bytes == len) ? 0 : -1;
}

int tcp_is_connected(tcp_state_t *state) {
    return state->connected;
}

void tcp_close(tcp_state_t *state) {
    state->running = 0;
    if (state->client_fd >= 0) close(state->client_fd);
    if (state->sock_fd >= 0) close(state->sock_fd);
    pthread_join(state->thread, NULL);
}

