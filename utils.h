#ifndef _UTILS_H
#define _UTILS_H

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>


#define LISTENQ 1024
#define BUFSIZE 8192
#define MAXEVENTS 1024

int create_and_bind(char *port);
int make_socket_non_blocking(int fd);
int open_listenfd(int port);

#endif
