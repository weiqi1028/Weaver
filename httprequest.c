#include "httprequest.h"


//*************************************************
//! Initialize the http request
/*!
\param request
    \li The http request to be initialized
\param fd
    \li The file descriptor
\return
    \li Always SERVER_OK
*/
//*************************************************
int http_request_init(http_request_t *request, int fd) {
    request->fd = fd;
    request->pos = request->last = request->buf;
    request->state = 0;
    request->head = (list_node_t *)malloc(sizeof(list_node_t));
    init_list(request->head);

    return SERVER_OK;
}

//*************************************************
//! Destroy the http request
/*!
\param request
    \li The http request to be destroyed
\return
    \li Always SERVER_OK
*/
//*************************************************
int http_request_destroy(http_request_t *request) {
    // TODO implement destroy http request function
    return SERVER_OK;
}

//*************************************************
//! Parse the http request line
/*!
\param request
    \li The http request pointer
\return
    \li SERVER_OK if the parse is done,
        SERVER_AGAIN if the request line is incomplete
        HTTP_PARSE_INVALID_METHOD or
        HTTP_PARSE_INVALID_REQUEST on error
*/
//*************************************************
int http_parse_request_line(http_request_t *request) {
    char ch, *p, *q;
    
    enum {
        start = 0,
        method,
        spaces_before_uri,
        after_slash_uri,
        http,
        http_H,
        http_HT,
        http_HTT,
        http_HTTP,
        before_major_digit,
        major_digit,
        before_minor_digit,
        minor_digit,
        spaces_after_digit,
        after_cr
    } state;
    
    state = request->state;
    debug("ready to parese request line, start = %d, last= %d", (int)request->pos, (int)request->last);
    
    for (p = request->pos; p < request->last; p++) {
        ch = *p;
        
        switch (state) {
        case start:
            request->request_start = p;
            if (ch == CR || ch == LF)
                break;
            if ((ch < 'A' || ch > 'Z') && ch != '_')
                return HTTP_PARSE_INVALID_METHOD;
            state = method;
            break;
            
        case method:
            if (ch == ' ') {
                request->method_end = p;
                q = request->request_start;
                switch (p - q) {
                case 3:
                    if (*q == 'G' && *(q + 1) == 'E' && *(q + 2) == 'T') {
                        request->method = HTTP_GET;
                        break;
                    }
                    break;
                
                default:
                    request->method = HTTP_UNKNOWN;
                    break;
                }
                state = spaces_before_uri;
                break;
            }
            
            if ((ch < 'A' || ch > 'Z') && ch != '_') {
                return HTTP_PARSE_INVALID_METHOD;
            }
            break;
            
        case spaces_before_uri:
            if (ch == '/') {
                request->uri_start = p;
                state = after_slash_uri;
                break;
            }
            switch (ch) {
            case ' ':
                break;
            default:
                return HTTP_PARSE_INVALID_REQUEST;
            }
            break;
            
        case after_slash_uri:
            switch (ch) {
            case ' ':
                request->uri_end = p;
                state = http;
                break;
            default:
                break;
            }
            break;
            
        case http:
            switch (ch) {
            case ' ':
                break;
            case 'H':
                state = http_H;
                break;
            default:
                return HTTP_PARSE_INVALID_REQUEST;
            }
            break;
            
        case http_H:
            switch (ch) {
            case 'T':
                state = http_HT;
                break;
            default:
                return HTTP_PARSE_INVALID_REQUEST;
            }
            break;
            
        case http_HT:
            switch (ch) {
            case 'T':
                state = http_HTT;
                break;
            default:
                return HTTP_PARSE_INVALID_REQUEST;
            }
            break;
            
        case http_HTT:
            switch (ch) {
            case 'P':
                state = http_HTTP;
                break;
            default:
                return HTTP_PARSE_INVALID_REQUEST;
            }
            break;
            
        case http_HTTP:
            switch (ch) {
            case '/':
                state = before_major_digit;
                break;
            default:
                HTTP_PARSE_INVALID_REQUEST;
            }
            break;
            
        case before_major_digit:
            if (ch < '1' || ch > '9')
                return HTTP_PARSE_INVALID_REQUEST;
            request->http_major = ch - '0';
            state = major_digit;
            break;
            
        case major_digit:
            if (ch == '.') {
                state = before_minor_digit;
                break;
            }
            if (ch < '0' || ch > '9')
                return HTTP_PARSE_INVALID_REQUEST;
            request->http_major = request->http_major * 10 + (ch - '0');
            
        case before_minor_digit:
            if (ch < '0' || ch > '9')
                return HTTP_PARSE_INVALID_REQUEST;
            request->http_minor = ch - '0';
            state = minor_digit;
            break;
            
        case minor_digit:
            if (ch == CR) {
                state = after_cr;
                break;
            }
            if (ch == LF) {
                goto done;
            }
            if (ch = ' ') {
                state = spaces_after_digit;
                break;
            }
            if (ch < '0' || ch > '9')
                return HTTP_PARSE_INVALID_REQUEST;
                
            request->http_minor = request->http_minor * 10 + (ch - '0');
            break;
            
        case spaces_after_digit:
            switch (ch) {
            case ' ':
                break;
            case CR:
                state = after_cr;
                break;
            case LF:
                goto done;
            default:
                return HTTP_PARSE_INVALID_REQUEST;
            }
            break;
            
        case after_cr:
            request->request_end = p - 1;
            switch (ch) {
            case LF:
                goto done;
            default:
                return HTTP_PARSE_INVALID_REQUEST;
            }
            break;
        }
    }
    
    request->pos = p;
    request->state = state;
    return SERVER_AGAIN;
    
done:
    request->pos = p + 1;
    if (request->request_end == NULL)
        request->request_end = p;
        
    request->state = start;
    
    return SERVER_OK;
}

//*************************************************
//! Parse the http request body
/*!
\param request
    \li The http request pointer
\return
    \li SERVER_OK if the parse is done,
        SERVER_AGAIN if the request body is incomplete
        HTTP_PARSE_INVALID_HEADER on error
*/
//*************************************************
int http_parse_request_body(http_request_t *request) {
    char ch, *p, *q;
    
    enum {
        header_start = 0,
        header_key,
        spaces_before_colon,
        spaces_after_colon,
        header_value,
        header_cr,
        header_crlf,
        header_crlfcr
    } state;
    
    state = request->state;
    debug("ready to parese request body, start = %d, last= %d", (int)request->pos, (int)request->last);
    
    http_header_t *hd;
    
    for (p = request->pos; p < request->last; p++) {
        ch = *p;
        
        switch (state) {
        case header_start:
            if (ch == CR || ch == LF)
                break;
            request->cur_header_key_start = p;
            state = header_key;
            break;
        
        case header_key:
            if (ch == ' ') {
                request->cur_header_key_end = p;
                state = spaces_before_colon;
                break;
            }
            if (ch == ':') {
                request->cur_header_key_end = p;
                state = spaces_after_colon;
                break;
            }
            break;
            
        case spaces_before_colon:
            if (ch == ' ')
                break;
            else if (ch == ':') {
                state = spaces_after_colon;
                break;
            }
            else
                return HTTP_PARSE_INVALID_HEADER;
                
        case spaces_after_colon:
            if (ch == ' ')
                break;
            state = header_value;
            request->cur_header_value_start = p;
            break;
            
        case header_value:
            if (ch == CR) {
                request->cur_header_value_end = p;
                state = header_cr;
            }
            if (ch == LF) {
                request->cur_header_value_end = p;
                state = header_crlf;
            }
            break;
            
        case header_cr:
            if (ch == LF) {
                state = header_crlf;
                hd = (http_header_t *)malloc(sizeof(http_header_t));
                hd->key_start = request->cur_header_key_start;
                hd->key_end = request->cur_header_key_end;
                hd->value_start = request->cur_header_value_start;
                hd->value_end = request->cur_header_value_end;
                list_node_t *node = (list_node_t *)malloc(sizeof(list_node_t));
                node->ptr = (void *)hd;
                list_tail_add(request->head, node);
                request->num_headers++;
                break;
            }
            else {
                return HTTP_PARSE_INVALID_HEADER;
            }
            
        case header_crlf:
            if (ch == CR) {
                state = header_crlfcr;
            }
            else {
                request->cur_header_key_start = p;
                state = header_key;
            }
            break;
            
        case header_crlfcr:
            switch (ch) {
            case LF:
                goto done;
            default:
                return HTTP_PARSE_INVALID_HEADER;
            }
            break;
        }
    }
    
    request->pos = p;
    request->state = state;
    return SERVER_AGAIN;
    
done:
    request->pos = p + 1;
    request->state = header_start;
    return SERVER_OK;
}
