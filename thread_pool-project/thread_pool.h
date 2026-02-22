#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H

#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>

typedef void (*handler_pt)(void *);

typedef struct task_s {
    handler_pt function;
    void *arg;
    struct task_s *next;
} task_t;

typedef struct threadpool_s {
    task_t *queue_head;
    task_t *queue_tail;
    int queue_size;
    int queue_max_size;
    pthread_t *threads;
    int thread_count;
    atomic_int shutdown;
    atomic_int started;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    pthread_cond_t wait_done;
} threadpool_t;

#ifdef __cplusplus
extern "C" {
#endif

    // 注意：这里修正了参数类型为 int (与你代码中的实现一致)
    threadpool_t *threadpool_create(int thread_count, int queue_max_size);
    void threadpool_terminate(threadpool_t* pool);
    
    // 注意：这里修正了函数名，与 main.c 中的调用一致
    int threadpool_post(threadpool_t* pool, handler_pt func, void *arg);
    void threadpool_waitdone(threadpool_t* pool);

#ifdef __cplusplus
}
#endif
#endif