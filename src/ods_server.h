#ifndef _ODS_SERVER_H
#define _ODS_SERVER_H_

#include "poller.h"

struct ods_server_params {
    char *addr;
    char *service;
    int qlen;
    int max_clients;
};

struct ods_server;
typedef int (*ods_action_t)(struct ods_server *server, void *data, int fd);

struct ods_fd_info {
    void *data;
    ods_action_t action;
};

struct ods_server {
    int fd;
    poller_t poller;
    struct ods_fd_info *fd_infos;
};

struct ods_accept_data {

};

struct ods_welcome_data {

};

int ods_accept(struct ods_server *server, struct ods_accept_data *data, int fd);
int ods_welcome(struct ods_server *server, struct ods_welcome_data *data, int fd);

#define SCHEDULE_FOR_READ (POLL_FOR_READ)
#define SCHEDULE_FOR_WRITE (POLL_FOR_WRITE)
void ods_server_set_data(struct ods_server *server, int fd, void *data);
void *ods_server_get_data(struct ods_server *server, int fd);
int ods_server_init(struct ods_server *server, struct ods_server_params *params);
int ods_server_loop(struct ods_server *server);
int ods_server_read(struct ods_server *server, int fd, ods_action_t action);
int ods_server_write(struct ods_server *server, int fd, char *buf, int n);
int ods_server_unschedule(struct ods_server *server, int fd);

#endif /* _ODS_SERVER_H_ */
