#include <stdlib.h>
#include <unistd.h>
#include "rio.h"

#define	MAXLINE	 8192  /* Max text line length */
#define SHORTLINE 512
#define MAXBUF   8192  /* Max I/O buffer size */

void *handle_http(void *arg);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
