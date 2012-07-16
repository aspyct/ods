#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "poller.h"

int poller_init(poller_t *poller) {
    *poller = epoll_create1(EPOLL_CLOEXEC);
    return *poller;
}

int poller_destroy(poller_t poller) {
    return close(poller);
};

int poller_add_server(poller_t poller, int fd) {
    struct epoll_event ev;

    ev.events = EPOLLIN;
    ev.data.fd = fd;
    return epoll_ctl(poller, EPOLL_CTL_ADD, fd, &ev);
}

int poller_add(poller_t poller, int fd, int events) {
    struct epoll_event ev;
    
    ev.events = events | EPOLLET;
    ev.data.fd = fd;
    return epoll_ctl(poller, EPOLL_CTL_ADD, fd, &ev);
}

int poller_remove(poller_t poller, int fd) {
    return epoll_ctl(poller, EPOLL_CTL_DEL, fd, NULL);
}

int poller_wait(poller_t poller, poller_ready ready[], int max, int timeout) {
    return epoll_wait(poller, ready, max, timeout);
}

