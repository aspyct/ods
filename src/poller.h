#ifndef _POLLER_H_
#define _POLLER_H_

/**
 * Well, this is platform dependent. Find a way to initialize this
 */

#include <sys/epoll.h>

typedef int poller_t;
typedef struct epoll_event poller_ready;

#define POLLER_READY_FD(x) ((x)->data.fd)

#define POLL_FOR_READ (EPOLLIN)
#define POLL_FOR_WRITE (EPOLLOUT)

/**
 * End of platform dependent stuff
 */

struct poller_ready_fd {
    int fd;
    void *data;
};

int poller_init(poller_t *poller);
int poller_destroy(poller_t poller);
int poller_add_server(poller_t poller, int fd);
int poller_add(poller_t poller, int fd, int events);
int poller_remove(poller_t poller, int fd);

/**
 * Returns the number of fd that are ready for either read of write
 * -1 on error
 *
 */
int poller_wait(poller_t poller, poller_ready ready[], int max, int timeout);

#endif

