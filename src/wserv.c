#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>

#include "wclient.h"
#include "poller.h"

struct server_params {
    char *addr;
    char *service;
    int qlen;
    int max_clients;
};

static void server(struct server_params *params);
static int create_server(struct server_params *params);

/**
 * For internal use by create_server()
 * Don't use directly
 */
static int make_server_socket(struct addrinfo *ai, int qlen);

int main(int argc, char *argv[]) {
    // Parse options from command line
    struct server_params params;

    bzero(&params, sizeof(params));
    params.addr = "127.0.0.1";
    if (argc > 1) {
        params.service = argv[1];
    }
    else {
        params.service = "http";
    }
    params.qlen = 10;
    params.max_clients = 100;

    server(&params);

    exit(0);
}

void server(struct server_params *params) {
    int server_fd;
    poller_t poller;
    poller_ready *ready_fds = calloc(params->max_clients, sizeof(poller_ready));

    server_fd = create_server(params);
    if (server_fd < 0) {
        fprintf(stderr, "Could not obtain server socket\n");
        exit(1);
    }
    else {
        int sock = accept(server_fd, NULL, NULL);
        close(sock);

        if (poller_init(&poller) < 0) {
            perror("Could not init poller");
            exit(2);
        }

        if (poller_add_server(&poller, server_fd) < 0) {
            perror("Could not add server to poller");
            exit(3);
        }
        
        for (;;) {
            // Careful, epoll is available on linux only
            // TODO Externalize this
            
            poller_wait(&poller, ready_fds, params->max_clients, 10000);
            printf("poller_wait() exited\n");
        }
    }
}

int create_server(struct server_params *params) {
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
        if ((sock_fd = make_server_socket(aip, params->qlen)) >= 0) {
            printf("Got socket for %s\n",
                    inet_ntoa(((struct sockaddr_in *)aip->ai_addr)->sin_addr));
            break;
        }
    }

    return sock_fd;
}

int make_server_socket(struct addrinfo *ai, int qlen) {
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

