#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../src/ods_http.h"

int main() {
    struct ods_http_request_parser parser;

    ods_http_request_parser_init(&parser, 4096);
    char *str = "GET /hello/there HTTP/1.1\r\n\r\n";
    ods_http_request_parser_feed(&parser, str, strlen(str));
    printf("Method: %s, URI: %s, Version: %s\n",
            parser.request->method,
            parser.request->uri,
            parser.request->version);

    ods_http_request_parser_init(&parser, 4096);
    ods_http_request_parser_feed(&parser, "GET", 3);
    if (parser.request->method != NULL) {
        fprintf(stderr, "The request should not have a method yet.\n");
        exit(1);
    }

    ods_http_request_parser_feed(&parser, " ", 1);
    if (parser.request->method == NULL) {
        fprintf(stderr, "The request should have the method now.\n");
        exit(2);
    }

    if (strcmp(parser.request->method, "GET") != 0) {
        fprintf(stderr, "The request method should be GET");
        exit(3);
    }
}

