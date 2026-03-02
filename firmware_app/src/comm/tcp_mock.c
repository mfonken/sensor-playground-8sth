#include "tcp_mock.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <poll.h>
#include <stdio.h>

int tcp_init(tcp_state_t *state)
{
    if (state->initialized) return 0;

    state->sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (state->sock_fd < 0) return -1;

    int opt = 1;
    setsockopt(state->sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_port        = htons(state->sock_port),
        .sin_addr.s_addr = INADDR_ANY
    };

    if (bind(state->sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(state->sock_fd);
        return -1;
    }

    listen(state->sock_fd, 5);
    state->running     = 1;
    state->initialized = 1;
    printf("TCP server ready on port %d\n", state->sock_port);
    return 0;
}

void tcp_manage_connection(tcp_state_t *state)
{
    if (!state->running) return;

    // Accept newest incoming connection, replacing any existing one
    struct pollfd pfd = { .fd = state->sock_fd, .events = POLLIN };
    if (poll(&pfd, 1, 0) > 0 && (pfd.revents & POLLIN)) {
        if (state->client_fd >= 0) {
            close(state->client_fd);
        }
        state->client_fd = accept(state->sock_fd, NULL, NULL);
        state->connected = (state->client_fd >= 0);
        printf("TCP %s\n", state->connected ? "connected" : "accept failed");
    }

    // Check if existing client has disconnected
    if (state->client_fd >= 0) {
        struct pollfd cfd = { .fd = state->client_fd, .events = POLLIN };
        int ret = poll(&cfd, 1, 0);
        char buf[1];
        if (ret > 0 && (cfd.revents & POLLIN) && recv(state->client_fd, buf, 1, MSG_PEEK) == 0) {
            close(state->client_fd);
            state->client_fd = -1;
            state->connected  = 0;
            printf("TCP disconnected\n");
        }
    }
}

int tcp_send(tcp_state_t *state, const char *data, int len)
{
    if (!state->connected || state->client_fd < 0) return -1;
    int bytes = send(state->client_fd, data, len, 0);
    return (bytes == len) ? 0 : -1;
}

int tcp_is_connected(tcp_state_t *state)
{
    return state->connected;
}

void tcp_close(tcp_state_t *state)
{
    state->running = 0;
    if (state->client_fd >= 0) close(state->client_fd);
    if (state->sock_fd >= 0)   close(state->sock_fd);
}