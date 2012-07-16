#ifndef _ODS_HTTP_H_
#define _ODS_HTTP_H_

#include <stdlib.h>
#include <string.h>

struct ods_http_one_header {
    char *header;
    char *value;
};

#define ODS_HTTP_HEADERS_MAX 50
struct ods_http_headers {
    // Array of header, value, header, value, etc.
    // Space for 50 headers should be enough
    struct ods_http_one_header items[ODS_HTTP_HEADERS_MAX];
    int count;
};

struct ods_http_request {
    char *method;
    char *uri;
    char *version;
    struct ods_http_headers headers;
};

struct ods_http_response {
    int status;
    char *body;
    int len;
    struct ods_http_headers headers;
};

struct ods_http_request_parser {
    struct ods_http_request *request;
    int offset;
    int parse_offset;
    char *buffer;
    int buf_size;
    enum {
        method,
        uri,
        version,
        headers,
        done
    } step;
};


/**
 * struct ods_http_request functions
 */
void ods_http_request_init(struct ods_http_request *self);
void ods_http_request_destroy(struct ods_http_request *self);
void ods_http_request_empty(struct ods_http_request *self);
int ods_http_request_set_method(struct ods_http_request *self, const char *method);
int ods_http_request_set_uri(struct ods_http_request *self, const char *uri);
int ods_http_request_set_version(struct ods_http_request *self, const char *version);

#define ods_http_request_method(x) ((x)->method)
#define ods_http_request_uri(x) ((x)->uri)
#define ods_http_request_version(x) ((x)->version)

/**
 * struct ods_http_request_parser functions
 */
void ods_http_request_parser_init(struct ods_http_request_parser *self,
                                  int buf_size);

/**
 * Returns 1 if the request is fully parsed
 *         0 if it needs more
 *        -1 in case of bad request
 *        -2 in case of internal error
 */
int ods_http_request_parser_feed(struct ods_http_request_parser *self,
                                 const char *food, int len);

int ods_http_headers_put(struct ods_http_headers *self,
                         const char *header, const char *value);
int ods_http_headers_del(struct ods_http_headers *self,
                         const char *header);

#define ods_http_headers_init(x)
#define ods_http_headers_start(x) (0)
#define ods_http_headers_end(x) (x->count)
#define ods_http_headers_at(x, i) (&(x)->items[(i)])
#define ods_http_headers_count(x) ((x)->count)
#define ods_http_headers_last(x) (&(x)->items[(x)->count - 1])

#endif /* _ODS_HTTP_H_ */
