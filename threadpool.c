#include "threadpool.h"


//*************************************************
//! Initialize the thread pool
/*!
\param num
    \li The number of threads in the thread pool
\return
    \li The pointer of the thread pool
*/
//*************************************************
threadpool_t *threadpool_init(int num) {
    threadpool_t *tp;
    tp = (threadpool_t *)malloc(sizeof(threadpool_t));
    if (tp == NULL) {
        log_err("thread pool init");
        abort();
    }
    
    tp->threads = (pthread_t *)malloc(sizeof(pthread_t) * num);
    if (tp->threads == NULL) {
        log_err("thread pool threads init");
        abort();
    }
    if (pthread_cond_init(&(tp->condition), NULL) != 0) {
        log_err("thread pool condition init");
        abort();
    }
    if (pthread_mutex_init(&(tp->lock), NULL) != 0) {
        log_err("thread pool mutex lock init");
        abort();
    }
    tp->head = (threadpool_task_t *)malloc(sizeof(threadpool_task_t));
    tp->head->function = NULL;
    tp->head->args = NULL;
    tp->head->next = NULL;
    tp->thread_count = 0;
    tp->queue_size = 0;
    tp->shutdown = 0;
    tp->started = 0;
    
    int i;
    for (i = 0; i < num; i++) {
        if (pthread_create(&(tp->threads[i]), NULL, threadpool_worker, (void *)tp) != 0) {
            log_err("creating worker thread");
            abort();
        }
        tp->started++;
        tp->thread_count++;
    }
    return tp;
}

//*************************************************
//! Add a new thread to the thread pool
/*!
\param tp
    \li The pointer of the thread pool
\param function
    \li The pointer of the function to be executed
\param args
    \li The pointer of the function arguments
\return
    \li 0 if success, -1 othrewise
*/
//*************************************************
int threadpool_add(threadpool_t *tp, void (*function)(void *), void *args) {
    if (tp == NULL || function == NULL) {
        log_err("invalid arguments in threadpool_addlog_err");
        return -1;
    }
    
    if (pthread_mutex_lock(&(tp->lock)) != 0) {
        log_err("mutex lock in threadpool_add");
        return -1;
    }
    
    if (tp->shutdown) {
        if (pthread_mutex_unlock(&(tp->lock)) != 0) {
            log_err("mutex unlock in threadpool_add");
            return -1;
        }
    }
    
    threadpool_task_t *task = (threadpool_task_t *)malloc(sizeof(threadpool_task_t));
    if (task == NULL) {
        log_err("malloc for task fail");
        return -1;
    }
    
    task->function = function;
    task->args = args;
    
    /* insert the new task at the head */
    task->next = tp->head->next;
    tp->head->next = task;
    
    tp->queue_size++;
    
    if (pthread_cond_signal(&(tp->condition)) != 0) {
        log_err("cond_signal in threadpool_add");
        return -1;
    }
    
    if (pthread_mutex_unlock(&(tp->lock)) != 0) {
        log_err("mutex unlock in threadpool_add");
        return -1;
    }
    
    return 0;
}

//*************************************************
//! Release the memory of the thread pool
/*!
\param tp
    \li The pointer of the thread pool
\return
    \li Always 0
*/
//*************************************************
static int threadpool_free(threadpool_t *tp) {
    if (tp == NULL || tp->started > 0)
        return -1;
    
    if (tp->threads)
        free(tp->threads);
        
    threadpool_task_t *ptask;
    while (tp->head->next) {
        ptask = tp->head->next;
        tp->head->next = tp->head->next->next;
        free(ptask);
    }
    
    return 0;
}

//*************************************************
//! Destroy the thread pool
/*!
\param tp
    \li The pointer of the thread pool
\return
    \li 0 if success, -1 otherwise
*/
//*************************************************
int threadpool_destroy(threadpool_t *tp) {
    if (tp->shutdown) {
        log_err("already shutdown");
        return -1;
    }
    
    tp->shutdown = 1;
    
    if (pthread_mutex_lock(&(tp->lock)) != 0) {
        log_err("lock in thread pool destroy");
        return -1;
    }
    
    if (pthread_cond_broadcast(&(tp->condition)) != 0) {
        log_err("cond_broadcast in thread pool destroy");
        return -1;
    }
    
    if (pthread_mutex_unlock(&(tp->lock)) != 0) {
        log_err("unlock in thread pool destroy");
        return -1;
    }
    
    int i;
    int rcode = 0;
    for (i = 0; i < tp->thread_count; i++) {
        if (pthread_join(tp->threads[i], NULL) != 0) {
            log_err("thread join in thread pool destroy");
            rcode = -1;
        }
    }
    
    if (!rcode) {
        pthread_mutex_destroy(&(tp->lock));
        pthread_cond_destroy(&(tp->condition));
        threadpool_free(tp);
    }
    return rcode;
}

//*************************************************
//! The thread worker function
/*!
\param args
    \li The pointer of the arguments
*/
//*************************************************
void *threadpool_worker(void *args) {
    threadpool_t *tp = (threadpool_t *)args;
    threadpool_task_t *ptask;
    
    while (1) {
        pthread_mutex_lock(&(tp->lock));
        while ((tp->queue_size == 0) && !(tp->shutdown)) {
            pthread_cond_wait(&(tp->condition), &(tp->lock));
        }
        
        if (tp->shutdown) {
            pthread_mutex_unlock(&(tp->lock));
            break;
        }
        
        ptask = tp->head->next;
        if (ptask == NULL) {
            continue;
        }
        tp->head->next = ptask->next;
        tp->queue_size--;
        pthread_mutex_unlock(&(tp->lock));
        
        /* execute the task */
        (*(ptask->function))(ptask->args);
        free(ptask);
    }
    
    tp->started--;
    pthread_mutex_unlock(&(tp->lock));
    pthread_exit(NULL);
    
    return NULL;
}
