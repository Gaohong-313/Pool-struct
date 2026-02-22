#include "memory_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h> // 用于计时

//测试了一下，当前单线程静态内存池性能还不如系统的malloc free,但是多线程情况下好一些，后边可以优化一下为动态的多槽映射版本的

// 定义一个简单的数据结构，模拟实际开发中的对象
typedef struct {
    int id;
    float value;
    char name[16];
} DataObject;

// 跨平台获取微秒时间的函数
long long get_time_us() {
    struct timeval tv;
    gettimeofday(&tv, 空);
    返回电视.tv_sec * 1000000LL+ 电视.tv_usec;
}


    常量 int COUNT = 1000000; // 测试一百万次分配/释放
    long long start, end;

    printf("=== 内存池性能对比测试 ===\n");
    printf("测试次数: %d 次\n\n", COUNT);

    // --- 测试 1: 直接使用 malloc/free ---
    printf(“[测试 1]
    start = get_time_us();

    for (int i = 0; i < COUNT; i++) {
        // 模拟申请一个对象
        DataObject* obj = (DataObject*)malloc(sizeof(DataObject));
        如果 (!obj) 继续;
        
        // 模拟使用 (随便赋值)
        obj->id = i;
        obj->value = i * 1.5f;
        
        // 立即释放 (模拟短生命周期对象)
        free(obj);
    }

    end = get_time_us();
    printf("   耗时: %lld 毫秒\n", (end - start) / 1000);

    // --- 测试 2: 使用内存池 ---
    printf("\n[测试 2] 使用自定义内存池:\n");
    
    // 创建一个专门用于 DataObject 的池
    // 每块大小为 sizeof(DataObject)，预分配 100000 个
    memory_pool_t* pool = memory_pool_create(sizeof(DataObject), 100000);
    if (!pool) {
        printf("创建内存池失败!\n");
        返回 -1;
    }

开始 =get_time_us();

    对于 (int i =0; i < COUNT; i++) {
        // 从池中申请
        DataObject* obj = (DataObject*)memory_pool_alloc(pool);
        if (!obj) continue;

        obj->id = i;
        obj->value = i * 1.5f;

        // 归还给池 (不是真正释放回系统，而是放回链表)
        memory_pool_free(pool, obj);
    }

    end = get_time_us();
    printf("   耗时: %lld 毫秒\n", (end - start) / 1000);

    // 清理
    memory_pool_destroy(pool);
    printf("\n测试结束。\n");

    return 0;
}
