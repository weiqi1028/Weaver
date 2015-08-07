#ifndef _RIO_H
#define _RIO_H

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

/* ref: CSAPP */
#define RIO_BUFSIZE 8192

/* Persistent state for the robust I/O (Rio) package */
typedef struct {
    int rio_fd;                 /* descriptor for this internal buf */
    int rio_cnt;                /* unread bytes in internal buf */
    char *rio_bufptr;           /* next unread byte in interal buf */
    char rio_buf[RIO_BUFSIZE];  /* internal buffer */
} rio_t;

/* Rio (Robust I/O) package */
ssize_t rio_readn(int fd, void *usrbuf, size_t n);
ssize_t rio_writen(int fd, void *usrbuf, size_t n);
void rio_readinitb(rio_t *rp, int fd); 
ssize_t	rio_readnb(rio_t *rp, void *usrbuf, size_t n);
ssize_t	rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);

#endif
