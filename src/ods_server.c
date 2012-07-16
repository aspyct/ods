#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <strings.h>

#include "ods_server.h"

#include "poller.h"

static int _make_server_socket(struct addrinfo *ai, int qlen);
static int _create_server(struct ods_server_params *params);

int ods_accept(struct ods_server *server, struct ods_accept_data *data, int fd) {
    int client_fd;
    struct sockaddr client_addr;
    socklen_t addrlen;

    client_fd = accept(fd, &client_addr, &addrlen);
    if (client_fd < 0) {
        perror("Could not accept client");
        return -1;
    }
    else {
        ods_server_set_data(server, client_fd, NULL);
        return ods_server_read(server, client_fd, (ods_action_t)ods_welcome);
    }
}

int ods_welcome(struct ods_server *server, struct ods_welcome_data *data, int fd) {
    return 0;
}

int ods_server_unschedule(struct ods_server *server, int fd) {
    if (poller_remove(server->poller, fd) < 0) {
        perror("Could not unschedule fd");
        return -1;
    }

    return 0;
}

int ods_server_read(struct ods_server *server, int fd, ods_action_t action) {
    if (poller_add(server->poller, fd, POLL_FOR_READ) < 0) {
        perror("Could not schedule read");
        return -1;
    }

    server->fd_infos[fd].action = action;
    return 0;
}

#define MAX_EVENTS 10
int ods_server_loop(struct ods_server *server) {
    poller_ready *ready_fds;
    poller_ready *p;
    poller_ready *end;
    int n;
    void *data;
    ods_action_t action;
    int fd;

    ready_fds = calloc(MAX_EVENTS, sizeof(poller_ready));

    ods_server_set_data(server, server->fd, NULL);
    ods_server_read(server, server->fd, (ods_action_t)ods_accept);

    for (;;) {
        printf("poller_wait()\n");
        n = poller_wait(server->poller, ready_fds, MAX_EVENTS, 10000);
        printf("poller_wait() exited\n");

        end = ready_fds + n;
        for (p = ready_fds; p < end; ++p) {
            fd = POLLER_READY_FD(p);
            action = server->fd_infos[fd].action;
            data = server->fd_infos[fd].data;
            action(server, data, fd);
        }
    }
}

int ods_server_init(struct ods_server *server, struct ods_server_params *params) {
    int open_max;

    open_max = sysconf(_SC_OPEN_MAX);
    if (open_max < 0) {
        open_max = 1024;
    }
    server->fd_infos = calloc(open_max, sizeof(struct ods_fd_info));

    server->fd = _create_server(params);
    if (server->fd < 0) {
        perror("Could not create server");
        return -1;
    }

    if (poller_init(&server->poller) < 0) {
        perror("Could not init poller");
        return -1;
    }

    return 0;
}

void ods_server_set_data(struct ods_server *server, int fd, void *data) {
    server->fd_infos[fd].data = data;
}

void *ods_server_get_data(struct ods_server *server, int fd) {
    return server->fd_infos[fd].data;   
}

int _create_server(struct ods_server_params *params) {
    int sock_fd = -1;
    struct addrinfo hint;
    struct addrinfo *ailist;
    struct addrinfo *aip;
    int err;

    // Get the address to serve
    bzero(&hint, sizeof(hint));
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = AI_NUMERICSERV;

    if ((err = getaddrinfo(params->addr, params->service, &hint, &ailist)) < 0) {
        printf("Could not get addr info: %s\n", gai_strerror(err));
        return -1;
    }

    for (aip = ailist; aip != NULL; aip = aip->ai_next) {
        if ((sock_fd = _make_server_socket(aip, params->qlen)) >= 0) {
            printf("Got socket for %s\n",
                    inet_ntoa(((struct sockaddr_in *)aip->ai_addr)->sin_addr));
            break;
        }
    }

    return sock_fd;
}

int _make_server_socket(struct addrinfo *ai, int qlen) {
    int sock_fd;
    int reuse = 1;

    sock_fd = socket(ai->ai_family, ai->ai_socktype, 0);
    if (sock_fd < 0) {
        perror("Could not create socket");
        return -1;
    }

    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0) {
        perror("Could not reuse socket");
        close(sock_fd);
        return -1;
    }

    if (bind(sock_fd, ai->ai_addr, ai->ai_addrlen) < 0) {
        perror("Could not bind socket to addr");
        close(sock_fd);
        return -1;
    }

    if (ai->ai_socktype == SOCK_STREAM || ai->ai_socktype == SOCK_SEQPACKET) {
        if (listen(sock_fd, qlen) < 0) {
            perror("Could not set the socket listening");
            close(sock_fd);
            return -1;
        }
    }

    // Waw, all good, terrific!
    return sock_fd;
}

