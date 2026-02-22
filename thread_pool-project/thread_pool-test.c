#define _POSIX_C_SOURCE 200809L
#include "thread_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void print_number(void *arg) {
    int num = *(int *)arg;
    printf("线程ID: %lu, 正在处理任务: %d\n", pthread_self(), num);
    sleep(1); // 模拟任务执行时间
    free(arg); // 任务执行完释放参数
}

int main() {
    #if 1
    threadpool_t *pool = threadpool_create(4, 0);
    if (!pool) {
        printf("创建线程池失败\n");
        return -1;
    }
#endif
    printf("线程池已启动，提交任务...\n");

    #if 1
    for (int i = 0; i < 10; i++) {
        int *arg = (int *)malloc(sizeof(int));
        *arg = i;
        threadpool_post(pool, print_number, arg);
    }

    printf("等待所有任务完成...\n");
    threadpool_waitdone(pool);
    printf("所有任务已完成！\n");

    threadpool_terminate(pool);
    getchar();
    printf("程序结束。\n");
    return 0;
    #endif
}