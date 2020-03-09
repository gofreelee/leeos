#ifndef MEMORY_H_
#define MEMORY_H_
#include "../lib/bitmap.h"
#include "../lib/stdint.h"
#define PG_P_1 1
#define PG_P_0 0
#define PG_RW_R 0
#define PG_RW_W 2
#define PG_US_U 4
#define PG_US_S 0

/*用来区分申请内存的是谁, 内核还是用户进程 */
enum pool_flags
{
    PF_KERNEL = 1,
    PF_USER = 2
};

//虚拟地址池数据结构
struct virtual_addrs
{
    struct bitmap vaddr_bitmap; // 用于管理内存的位图
    uint32_t *vaddr_start;      // 虚拟地址池的起始位置
};

extern struct pool kernel_pool, user_pool;
void mem_init();
void *malloc_page(enum pool_flags PF, uint32_t cnt_pages);
static void *vaddr_get_pages(enum pool_flags PF, uint32_t cnt);
#endif