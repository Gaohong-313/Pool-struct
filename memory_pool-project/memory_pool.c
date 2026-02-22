#include "memory_pool.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h> //互斥锁

//内存池结构体
struct memory_pool_s{

    size_t block_size; //每个内存块的大小
    size_t num_blocks; //内存块的数量
    void *memory; //内存池起始地址
    void **free_list; //空闲链表的头指针
    size_t free_count; //空闲的块数量

    pthread_mutex_t lock; //互斥锁
};

//宏定义简化锁代码
#define LOCK(pool) pthread_mutex_lock(&(pool)->lock) //加锁
#define UNLOCK(pool) pthread_mutex_unlock(&(pool)->lock) //解锁

memory_pool_t* memory_pool_create(size_t block_size, size_t num_blocks){

    if(block_size <= 0 || num_blocks <= 0){
        return NULL;
    }

    memory_pool_t *pool = NULL;

    pool = (memory_pool_t*)malloc(sizeof(memory_pool_t));
    if(pool == NULL){
        return NULL;
    }

    //内存对齐，确保block_size是指针的倍数
         // 7 % 4  = 3    
    if(block_size % sizeof(void*) != 0){
        block_size += sizeof(void*) - (block_size % sizeof(void*));
    }   //4  - 3 = 1;  7 + 1 = 8得到块大小为8

    //分配一大块连续内存

    void *memory = malloc(block_size * num_blocks); //分配一大块连续内存
    if(memory == NULL){
        free(pool);
        return NULL;
    }

    //初始化内存池结构体
    pool->block_size = block_size;
    pool->num_blocks = num_blocks;
    pool->memory = memory;
    pool->free_list = (void**)memory; //空闲链表的头指针
    pool->free_count = num_blocks; //空闲的块数量


    //初始化空闲链表
    pool->free_list = (void**)malloc(num_blocks * sizeof(void*)); //分配空闲链表
    if(pool->free_list == NULL){
        free(memory);
        free(pool);
        return NULL;
    }

    //将内存块加入空闲链表
    for(size_t i = 0; i < num_blocks; i++){

        void *block = (void*)((char*)memory + i * block_size); //计算内存块地址
        pool->free_list[i] = block; //加入空闲链表
    }

    //初始化互斥锁
    pthread_mutex_init(&pool->lock, NULL);

    printf("内存池创建成功！\n");
    return pool;
    
}

void *memory_pool_alloc(memory_pool_t *pool){
    if(pool == NULL){
        return NULL;
    }

    LOCK(pool); //加锁

    //取出空闲链表的第一个块
    void *block = pool->free_list[pool->free_count--]; //取出第一个块

    UNLOCK(pool); //解锁

    return block;
}

void memory_pool_free(memory_pool_t *pool, void *ptr){
   if(!pool || !ptr){
       return;
   }

   LOCK(pool); //加锁

   //将块加入空闲链表
   pool->free_list[pool->free_count++] = ptr; //加入空闲链表

   UNLOCK(pool); //解锁

   return;
}


void memory_pool_destroy(memory_pool_t *pool){

    if(pool == NULL){
        return;
    }

    //销毁互斥锁
    pthread_mutex_destroy(&pool->lock);

    //释放内存池
    free(pool->memory);
    free(pool->free_list);
    free(pool);

    printf("内存池销毁成功！\n");
    return;
}

