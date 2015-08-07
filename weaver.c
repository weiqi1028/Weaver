#include <sys/epoll.h>
#include <signal.h>
#include "utils.h"
#include "threadpool.h"
#include "httprequest.h"
#include "httphandler.h"


int main(int argc, char *argv[]) {
    printf("%s", "start\n");
    int listen_fd;
    int rcode;
    struct epoll_event *events;
    
    if (argc != 2) {
        fprintf(stderr, "usage: %s [port]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    if (sigaction(SIGPIPE, &sa, NULL)) {
        printf("ignore SIGPIPE\n");
    }
    
    struct sockaddr_in client_addr;
    socklen_t client_len = 1;
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    
    /* create and bind the port, and then set the socket to non blocking mode */
    listen_fd = open_listenfd(atoi(argv[1]));
    debug("listen fd = %d", listen_fd);
    rcode = make_socket_non_blocking(listen_fd);
    if (rcode == -1) {
        log_err("error when making socket non blocking");
        abort();
    }
    
    /* create epoll event */
    int efd = epoll_create1(0);
    if (efd == -1) {
        log_err("epoll_create");
        abort();
    }
    struct epoll_event event;

    events = (struct epoll_event *)malloc(sizeof(struct epoll_event) * MAXEVENTS);
    
    http_request_t *request = (http_request_t *)malloc(sizeof(http_request_t));
    http_request_init(request, listen_fd);
    
    event.data.ptr = (void *)request;
    event.events = EPOLLIN | EPOLLET;
    
    /* register the listen event */
    rcode = epoll_ctl(efd, EPOLL_CTL_ADD, listen_fd, &event);
    if (rcode == -1) {
        perror("epoll_ctl");
        abort();
    }
    
    threadpool_t *tp = threadpool_init(NUM_OF_THREADS);
    
    /* event loop */
    while (1) {
        int n = epoll_wait(efd, events, MAXEVENTS, -1);
        
        /* process each incoming IO event */
        int i;
        for (i = 0; i < n; i++) {
            http_request_t *r = (http_request_t *)events[i].data.ptr;
            int fd = r->fd;
            debug("event fd = %d", fd);
            if (fd == listen_fd) {  /* incoming connection event */
                
                while (1) {
                    int client_fd;
                    debug("waiting for accept");
                    client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
                    
                    if (client_fd == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            // we have already processed the incoming connection
                            debug("incoming connection processed\n");
                            break;
                        }
                        else {
                            log_err("error occured when accepting connection\n");
                            break;
                        }
                    }
                    
                    rcode = make_socket_non_blocking(client_fd);
                    if (rcode == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            // we have already processed the incoming connection
                            break;
                        log_err("fail to accept the connection\n");
                        break;
                    }
                    debug("new connection fd %d", client_fd);
                    
                    http_request_t *request = (http_request_t *)malloc(sizeof(http_request_t));
                    http_request_init(request, client_fd);
                    event.data.ptr = (void *)request;
                    event.events = EPOLLIN | EPOLLET;
                    /* add the new event into epoll */
                    rcode = epoll_ctl(efd, EPOLL_CTL_ADD, client_fd, &event);
                    if (rcode == - 1) {
                        log_err("fail in epoll_ctl in epoll_wait");
                        abort();
                    }
                }
                debug("end accept");
            }
            else if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || (!(events[i].events & EPOLLIN))) {
                /* an error has occured on this fd, or the socket is not ready for reading */
                log_err("error events: %d", events[i].events);
                if (events[i].events & EPOLLERR)
                    log_err("EPOLLERR");
                if (events[i].events & EPOLLHUP)
                    log_err("EPOLLHUP");
                if (!(events[i].events & EPOLLIN))
                    log_err("EPOLLIN");
                close(fd);
                continue;
            }
            else {  /* incoming data read event */
                /* add the event to the thread pool list */
                threadpool_add(tp, handle_http, events[i].data.ptr);
                debug("thread count: %d", tp->thread_count);
                debug("thread queue size: %d", tp->queue_size);
            }
        }
    }
    
    threadpool_destroy(tp);
    return 0;
}
