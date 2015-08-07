#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include "httphandler.h"
#include "httprequest.h"

//*************************************************
//! Function to handle the http request
/*!
\param arg
    \li The pointer of the http request argument
*/
//*************************************************
void *handle_http(void *arg) {
    http_request_t *request = (http_request_t *)arg;
    int fd = request->fd;
    int rcode = 0;
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[SHORTLINE], header_key[MAXLINE], header_value[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    int n;
    
    /* Read request line and headers */
    while (1) {
        printf("[INFO] read from fd %d\n", fd);
        n = read(fd, request->last, (uint64_t)request->buf + MAX_BUF - (uint64_t)request->last);
        printf("[INFO] n = %d\n", n);
        if (n == 0) {
            goto err;
        }
        
        if (n < 0) {
            if (errno != EAGAIN) {
                log_err("read error, errno = %d", errno);
                goto err;
            }
            break;
        }
        
        request->last += n;
        
        rcode = http_parse_request_line(request);
        if (rcode == SERVER_AGAIN) {
            continue;
        }
        else if (rcode != SERVER_OK) {
            log_err("parse request line error, rcode = %d", rcode);
            goto err;
        }
        
        strncpy(method, request->request_start, request->method_end - request->request_start);
        strncpy(uri, request->uri_start, request->uri_end - request->uri_start);
        sprintf(version, "%d.%d", request->http_major, request->http_minor);
        
        if (request->method != HTTP_GET) {
        
            debug("error method: %s\n", method);
            debug("error uri: %s\n", uri);
            debug("error version: %s\n", version);
            clienterror(fd, method, "501", "Not Implemented",
                    "Server does not implement this method");
            return NULL;
        }
        
        debug("http request line");
        debug("method = %s", method);
        debug("uri = %s", uri);
        debug("version = %s", version);
        
        rcode = http_parse_request_body(request);
        if (rcode == SERVER_AGAIN) {
            continue;
        }
        else if (rcode != SERVER_OK) {
            log_err("parse request body error, rcode = %d", rcode);
            goto err;
        }
        
        debug("number of headers = %d", request->num_headers);
        
        if (request->num_headers > 0) {
            list_node_t *head = request->head->next;
            int i;
            for (i = 0; i < request->num_headers; i++) {
                http_header_t *header = (http_header_t *)head->ptr;
                memset(header_key, 0, sizeof(header_key));
                memset(header_value, 0, sizeof(header_value));
                memcpy(header_key, header->key_start, header->key_end - header->key_start);
                memcpy(header_value, header->value_start, header->value_end - header->value_start);
                debug("%s: %s", header_key, header_value);
                head = head->next;
            }
        }
        
        is_static = parse_uri(uri, filename, cgiargs);
        
        if (stat(filename, &sbuf) < 0) {
	        clienterror(fd, filename, "404", "Not found",
		        "Server couldn't find this file");
	        continue;
	    }

        if (is_static) {
	        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
	            clienterror(fd, filename, "403", "Forbidden",
			        "Server couldn't read the file");
	            continue;
	        }
	        serve_static(fd, filename, sbuf.st_size);
        }
        
        goto close;
    }
    
    return NULL;

err:
close:
    debug("closing fd %d", fd);
    close(fd);
}


//*************************************************
//! Parse the uir in the request
/*!
\param uri
    \li The uri in the http  request
\param filename
    \li the string modified to contain the file name
\param cgiargs
    \li modified to contain the cgi arguments
\return
    \li 1 if it is static content, 0 if dynamic
*/
//*************************************************
int parse_uri(char *uri, char *filename, char *cgiargs)
{
    char *ptr;

    if (!strstr(uri, "cgi-bin")) {  /* Static content */
	    strcpy(cgiargs, "");
	    strcpy(filename, ".");
	    strcat(filename, uri);
	    if (uri[strlen(uri)-1] == '/')
	        strcat(filename, "home.html");
	    return 1;
    }
    else {  /* Dynamic content */
	    ptr = index(uri, '?');
	    if (ptr) {
	        strcpy(cgiargs, ptr+1);
	        *ptr = '\0';
	    }
	    else 
	        strcpy(cgiargs, "");
	    strcpy(filename, ".");
	    strcat(filename, uri);
	    return 0;
    }
}

//*************************************************
//! Server the static request
/*!
\param fd
    \li The file descriptor
\param filename
    \li The file name on the server
\param filesize
    \li The file size
*/
//*************************************************
void serve_static(int fd, char *filename, int filesize)
{
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXBUF];
 
    /* Send response headers to client */
    get_filetype(filename, filetype);
    sprintf(buf, "HTTP/1.1 200 OK\r\n");
    sprintf(buf, "%sServer Verver Web Server\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    rio_writen(fd, buf, strlen(buf));
    memset(buf, 0, sizeof(buf));

    /* Send response body to client */
    srcfd = open(filename, O_RDONLY, 0);
    srcp = mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    close(srcfd);
    rio_writen(fd, srcp, filesize);
    munmap(srcp, filesize);
}

//*************************************************
//! Get the file type
/*!
\param filename
    \li The file name
\param filetype
    \li Modified to contains the file type
*/
//*************************************************
void get_filetype(char *filename, char *filetype)
{
    if (strstr(filename, ".html"))
	    strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
	    strcpy(filetype, "image/gif");
    else if (strstr(filename, ".jpg"))
	    strcpy(filetype, "image/jpeg");
    else
	    strcpy(filetype, "text/plain");
}  

//*************************************************
//! Handle the client error
/*!
\param fd
    \li The file descriptor
\param cause
    \li The cause of the error
\param errnum
    \li The error number
\param shortmsg
    \li The short error message
\param longmsg
    \li The The long error message
*/
//*************************************************
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) {
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Server Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Verver Web server</em>\r\n", body);

    /* Print the HTTP response */
    debug("client error! return code %s", errnum);
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    rio_writen(fd, buf, strlen(buf));
    rio_writen(fd, body, strlen(body));
    printf("%s\n", buf);
    printf("%s\n", body);
}

