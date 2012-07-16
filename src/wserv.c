#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>

#include "ods_server.h"
#include "wclient.h"
#include "poller.h"

void serve(struct ods_server_params *params);

int main(int argc, char *argv[]) {
    // Parse options from command line
    struct ods_server_params params;

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

    serve(&params);

    exit(0);
}

void serve(struct ods_server_params *params) {
    struct ods_server server;
    if (ods_server_init(&server, params) < 0) {
        perror("Could not initialize server");
        exit(1);
    }

    ods_server_loop(&server); 
}


