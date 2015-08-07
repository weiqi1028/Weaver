#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "dbg.h"

#define NUM_OF_THREADS 4

//! The thread pool task struct
typedef struct threadpool_task {
    void (*function)(void *);
    void *args;
    struct threadpool_task *next;
} threadpool_task_t;

//! The thread pool struct
typedef struct {
    pthread_t *threads;
    pthread_mutex_t lock;
    pthread_cond_t condition;
    threadpool_task_t *head;
    int thread_count;
    int queue_size;
    int shutdown;
    int started;
} threadpool_t;

threadpool_t *threadpool_init(int num);
void *threadpool_worker(void *args);
int threadpool_add(threadpool_t *tp, void (*function)(void *), void *args);
int threadpool_destroy(threadpool_t *tp);

#endif
