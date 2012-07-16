#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "ods_http.h"

static int _parse_method(struct ods_http_request_parser *self);
static int _parse_uri(struct ods_http_request_parser *self);
static int _parse_version(struct ods_http_request_parser *self);
static int _parse_one_header(struct ods_http_request_parser *self);
static int _is_uri_char(char c);

void ods_http_request_parser_init(struct ods_http_request_parser *self,
                                  int buf_size) {
    self->buffer = malloc(buf_size);
    self->buf_size = buf_size;
    self->offset = 0;
    self->parse_offset = 0;
    self->step = method;

    self->request = malloc(sizeof(struct ods_http_request));
    ods_http_request_init(self->request);
}

int ods_http_request_parser_feed(struct ods_http_request_parser *self,
                                 const char *food, int len) {
    int overflow = 0;
    int pr;

    // first, copy data
    if (self->offset + len > self->buf_size - 1) {
        // Avoid buffer overflow, but try to parse the request
        // Should try compressing the buffer first, remove what has been parsed
        len = self->buf_size - self->offset - 1;
        overflow = 1;
    }
    strncpy(self->buffer + self->offset, food, len);
    self->offset += len;

    // Now try and parse something
    parse:
    switch (self->step) {
        case method:
            if ((pr = _parse_method(self)) == 1) {
                goto parse;
            }
            break;
        case uri:
            if ((pr = _parse_uri(self)) == 1) {
                goto parse;
            }
            break;
        case version:
            if ((pr = _parse_version(self)) == 1) {
                goto parse;
            }
            break;
        case headers:
            if ((pr = _parse_one_header(self)) == 1) {
                goto parse;
            }
            break;
        case done:
            // All done, return happily :)
            return 1;
    }

    return pr;
}

int _parse_method(struct ods_http_request_parser *self) {
    char *start;
    char *p;

    start = self->buffer + self->parse_offset;
    p = start;
    
    // Method must start with a letter
    if (!isalpha(*p)) {
        return -1;
    }
    
    // Find the first non-alpha char
    do {
        ++p;
    } while (isalpha(*p));

    // Next char should be ' '
    if (*p == ' ') {
        // Yup, we've got the method
        // Cut the char* and set it as method
        *p = '\0';
        self->parse_offset += p - start + 1; // go past the '\0'
        self->step = uri;
        ods_http_request_set_method(self->request, start);

        return 1;
    }
    else {
        // Oops... missing data ?
        return 0;
    }
}

int _parse_uri(struct ods_http_request_parser *self) {
    char *initial_offset;
    char *start;
    char *p;
    char prev;

    initial_offset = self->buffer + self->parse_offset;
    
    // Skip any whitespace
    for (p = initial_offset; *p == ' '; ++p) {}
    start = p;

    // First char must be '/'
    if (*p != '/') {
        return -1;
    }
    
    // We have to ensure that no '..' is in the url, hence the char prev
    prev = *p;
    
    do {
         if (prev == '.' && *p == '.') {
             // Potential attack ?
             return -1;
         }
         ++p;
    } while (_is_uri_char(*p));

    // Next char should be ' '
    if (*p == ' ') {
        *p = '\0';
        self->parse_offset += p - initial_offset + 1;
        self->step = version;
        ods_http_request_set_uri(self->request, start);

        return 1;
    }
    else {
        return 0;
    }
}

int _is_uri_char(char c) {
    if (isalpha(c) || isdigit(c)) {
        return 1;
    }
    else {
        // TODO Is this complete ?
        return strchr("/_-.~?&%", c) != NULL;
    }
}

int _parse_version(struct ods_http_request_parser *self) {
    char *initial_offset;
    char *start;
    char *p;

    initial_offset = self->buffer + self->parse_offset;

    // Skip any whitespaces
    for (p = initial_offset; *p == ' '; ++p) {}
    start = p;

    // Waiting for HTTP/x.x, where x is a single digit
    #define require_char(c, C) if (!(*p == (c) || *p == (C))) { return -1; } ++p;
    require_char('h', 'H');
    require_char('t', 'T');
    require_char('t', 'T');
    require_char('p', 'P');
    #undef require_char

    if (*p != '/') {
        return -1;
    }
    ++p;

    if (!isdigit(*p)) {
        return -1;
    }
    ++p;

    if (*p != '.') {
        return -1;
    }
    ++p;

    if (!isdigit(*p)) {
        return -1;
    }
    ++p;

    // Now *p should be either \r or \n
    if (*p == '\0') {
        return 0;
    }

    if (!(*p == '\r' || *p == '\n')) {
        return -1;
    }

    *p = '\0';

    if (ods_http_request_set_version(self->request, start) < 0) {
        return -2;
    }
    
    self->parse_offset += p - initial_offset + 1;
    self->step = headers;

    return 1;
}

int _parse_one_header(struct ods_http_request_parser *self) {
    // Later...
    self->step = done;
    return 1;
}

void ods_http_request_init(struct ods_http_request *self) {
    bzero(self, sizeof(struct ods_http_request));
}

int ods_http_request_set_method(struct ods_http_request *self, const char *method) {
    if ((self->method = realloc(self->method, strlen(method) + 1)) == NULL) {
        return -1;
    }

    strcpy(self->method, method);
    return 0;
}

int ods_http_request_set_uri(struct ods_http_request *self, const char *uri) {
    if ((self->uri = realloc(self->uri, strlen(uri) + 1)) == NULL) {
        return -1;
    }

    strcpy(self->uri, uri);
    return 0;
}

int ods_http_request_set_version(struct ods_http_request *self, const char *version) {
    if ((self->version = realloc(self->version, strlen(version) + 1)) == NULL) {
        return -1;
    }

    strcpy(self->version, version);
    return 0;
}

void ods_http_request_destroy(struct ods_http_request *self) {
    ods_http_request_empty(self);
    free(self);
}

void ods_http_request_empty(struct ods_http_request *self) {
    free(self->method);
    free(self->uri);
    free(self->version);
}

int ods_http_headers_put(struct ods_http_headers *self,
                         const char *header, const char *value) {
    struct ods_http_one_header *item;
    char *_header;
    char *_value;

    if (self->count < ODS_HTTP_HEADERS_MAX) {
        item = &self->items[self->count];

        if ((_header = malloc(strlen(header) + 1)) == NULL) {
            // OOM ?
            return -1;
        }

        if ((_value = malloc(strlen(value) + 1)) == NULL) {
            // OOM ?
            free(_header);
            return -1;
        }
        
        strcpy(_header, header);
        strcpy(_value, value);

        item->header = _header;
        item->value = _value;
        ++self->count;

        return 0;
    }
    else {
        // No more space
        return -1;
    }
}

int ods_http_headers_del(struct ods_http_headers *self,
                         const char *header) {
    int i;
    struct ods_http_one_header *item;

    for (i = ods_http_headers_start(self); i < ods_http_headers_end(self); ++i) {
        item = ods_http_headers_at(self, i);

        if (strcmp(item->header, header) == 0) {
            // Got it !
            memcpy(item, ods_http_headers_last(self),
                   sizeof(struct ods_http_one_header));
            --self->count;
            return 0;
        }
    }

    // Not found
    return -1;
}

