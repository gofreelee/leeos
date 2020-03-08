#ifndef MEMORY_H_
#define MEMORY_H_
#include "../lib/bitmap.h"
#include "../lib/stdint.h"

//虚拟地址池数据结构
struct virtual_addrs
{
    struct bitmap vaddr_bitmap; // 用于管理内存的位图
    uint32_t* vaddr_start; // 虚拟地址池的起始位置
};

extern struct pool kernel_pool, user_pool;
void mem_init();

#endif