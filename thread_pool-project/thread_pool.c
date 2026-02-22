#define _POSIX_C_SOURCE 200809L
#include "thread_pool.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

static void* worker_thread(void* arg){
    threadpool_t* pool = (threadpool_t*)arg;
    task_t* task = NULL;

    atomic_fetch_add(&pool->started, 1);

    while(1){
        pthread_mutex_lock(&pool->lock);

        // 如果队列为空且未关闭，等待
        while (pool->queue_size == 0 && !pool->shutdown) {
            pthread_cond_wait(&pool->not_empty, &pool->lock);
        }

        // 如果收到关闭信号，退出
        if (pool->shutdown) {
            pthread_mutex_unlock(&pool->lock);
            break;
        }

        // 取出任务
        task = pool->queue_head;
        pool->queue_head = task->next;
        pool->queue_size--;

        // 如果取走任务后队列空了，广播通知等待空队列的线程
        if (pool->queue_size == 0) {
            pthread_cond_broadcast(&pool->wait_done);
        }

        pthread_mutex_unlock(&pool->lock);

        // 执行任务 (注意：这里要在锁外执行，避免阻塞其他任务提交)
        if(task){
            task->function(task->arg);
            free(task);
        }
    }

    atomic_fetch_sub(&pool->started, 1);
    pthread_mutex_unlock(&pool->lock); // 确保锁被释放
    pthread_exit(NULL);
    return NULL;
}

threadpool_t* threadpool_create(int thread_count, int queue_max_size){
    threadpool_t* pool = NULL;
    int i, err;

    if((pool = (threadpool_t*)malloc(sizeof(threadpool_t))) == NULL){
        goto err;
    }

    pool->thread_count = thread_count;
    pool->queue_max_size = queue_max_size;
    pool->queue_size = 0;
    pool->queue_head = NULL;
    pool->queue_tail = NULL;
    atomic_init(&pool->shutdown, 0);
    atomic_init(&pool->started, 0);

    if((pool->threads = (pthread_t*)malloc(sizeof(pthread_t) * thread_count)) == NULL){
        goto err_free_pool;
    }

    if (pthread_mutex_init(&(pool->lock), NULL) != 0) goto err_free_threads;
    if (pthread_cond_init(&(pool->not_empty), NULL) != 0) goto err_destroy_mutex;
    if (pthread_cond_init(&(pool->not_full), NULL) != 0) goto err_destroy_not_empty;
    if (pthread_cond_init(&(pool->wait_done), NULL) != 0) goto err_destroy_not_full;

    for(i = 0; i < thread_count; i++){
        if((err = pthread_create(&pool->threads[i], NULL, worker_thread, (void*)pool)) != 0){
            fprintf(stderr, "pthread_create error: %s\n", strerror(err));
            threadpool_terminate(pool);
            return NULL;
        }
    }

    // 修正：usleep 的参数是微秒，1000 表示 1毫秒
    while(atomic_load(&pool->started) != thread_count){
        sleep(0.001); //
    }
    return pool;

err_destroy_not_full:
    pthread_cond_destroy(&pool->not_full);
err_destroy_not_empty:
    pthread_cond_destroy(&pool->not_empty);
err_destroy_mutex:
    pthread_mutex_destroy(&pool->lock);
err_free_threads:
    free(pool->threads);
err_free_pool:
    free(pool);
err:
    return NULL;
}

// 修正：函数名改为 threadpool_post，与头文件和 main.c 一致
int threadpool_post(threadpool_t* pool, handler_pt func, void *arg){
    task_t* task = NULL;

    if (!pool || !func) return -1;

    if((task = (task_t*)malloc(sizeof(task_t))) == NULL){
        return -1;
    }
    
    if(atomic_load(&pool->shutdown)){
        free(task);
        return -1;
    }

    task->function = func;
    task->arg = arg;
    task->next = NULL;

    pthread_mutex_lock(&pool->lock);

    // 如果设置了队列上限且队列已满，等待
    if (pool->queue_max_size > 0) {
        while (pool->queue_size >= pool->queue_max_size) {
            pthread_cond_wait(&pool->not_full, &pool->lock);
        }
    }

    if(pool->queue_size == 0){
        pool->queue_head = task;
        pool->queue_tail = task;
    } else {
        pool->queue_tail->next = task;
        pool->queue_tail = task;
    }
    pool->queue_size++;

    pthread_cond_signal(&pool->not_empty);
    pthread_mutex_unlock(&pool->lock);

    return 0;
}

// 修正：函数名改为 threadpool_waitdone，与头文件和 main.c 一致
void threadpool_waitdone(threadpool_t* pool){
    if (!pool) return;

    pthread_mutex_lock(&pool->lock);
    // 等待直到队列为空
    while (pool->queue_size != 0) {
        pthread_cond_wait(&pool->wait_done, &pool->lock);
    }
    pthread_mutex_unlock(&pool->lock);
}

void threadpool_terminate(threadpool_t* pool){
    if (!pool || atomic_load(&pool->shutdown)) return;

    atomic_store(&pool->shutdown, 1);

    pthread_mutex_lock(&pool->lock);
    pthread_cond_broadcast(&pool->not_empty);
    pthread_mutex_unlock(&pool->lock);

    for(int i = 0; i < pool->thread_count; i++){
        pthread_join(pool->threads[i], NULL);
    }

    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->not_empty);
    pthread_cond_destroy(&pool->not_full);
    pthread_cond_destroy(&pool->wait_done);
    free(pool->threads);
    free(pool);
}