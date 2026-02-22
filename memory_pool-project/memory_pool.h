#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <stdlib.h>
#include <stdint.h>

//封装内存池的结构体
typedef struct memory_pool_s memory_pool_t;


//brief 创建一个固定大小的内存池

//block_size: 每个内存块的大小

//num_blocks: 内存池中内存块的数量

//返回一个内存池的指针，失败返回NULL

memory_pool_t* memory_pool_create(size_t block_size, size_t num_blocks);//创建内存池

void *memory_pool_alloc(memory_pool_t *pool); //从内存池中分配一个内存块

void memory_pool_free(memory_pool_t *pool, void *ptr); //释放一个内存块到内存池中

void memory_pool_destroy(memory_pool_t *pool); //销毁一个内存池


#endif