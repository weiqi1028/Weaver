#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "list.h"
#include "dbg.h"

#define SERVER_AGAIN    EAGAIN
#define SERVER_OK       0

#define HTTP_PARSE_INVALID_METHOD        10
#define HTTP_PARSE_INVALID_REQUEST       11
#define HTTP_PARSE_INVALID_HEADER        12

#define HTTP_UNKNOWN                     0x0001
#define HTTP_GET                         0x0002
#define HTTP_HEAD                        0x0004
#define HTTP_POST                        0x0008

#define HTTP_OK                          200
#define HTTP_NOT_MODIFIED                304
#define HTTP_NOT_FOUND                   404

#define MAX_BUF 8124

#define CR '\r'
#define LF '\n'
#define CRLFCRLF "\r\n\r\n"

//! Http request struct
typedef struct http_request {
    int fd;
    char buf[MAX_BUF];
    void *pos;
    void *last;
    int state;
    void *request_start;
    void *method_end;   // not include method_end
    int method;
    void *uri_start;
    void *uri_end;      // not include uri_end
    void *path_start;
    void *path_end;
    void *query_start;
    void *query_end;
    int http_major;
    int http_minor;
    void *request_end;

    int num_headers;    // the number of the nodes in the list
    list_node_t *head;  // the http headers list head
    void *cur_header_key_start;
    void *cur_header_key_end;
    void *cur_header_value_start;
    void *cur_header_value_end;

} http_request_t;

//! Http header struct (key: value)
typedef struct http_header {
    void *key_start;
    void *key_end;
    void *value_start;
    void *value_end;
} http_header_t;


int http_request_init(http_request_t *request, int fd);
int http_request_destroy(http_request_t *request);
int http_parse_request_line(http_request_t *request);
int http_parse_request_body(http_request_t *request);

#endif

