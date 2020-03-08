#include "memory.h"
#include "../lib/kernel/print.h"
#include "../lib/stdint.h"

#define PAGE_SIZE 4096 // 一个页的大小为 4KB

#define KERNEL_HEAP_START 0xc0100000 // 我们设置的内核的堆的起始地址,跨过低端1MB

#define KERNEL_BITMAP_START 0xc009a000
/*内存池结构 */
struct pool
{
    struct bitmap pool_bitmap;
    uint32_t *phy_start_addr;
    uint32_t mem_size; // 内存池长度
};

struct pool kernel_pool, user_pool;
struct virtual_addrs kernel_vaddr;

static void mem_pool_init(uint32_t all_mem)
{
    uint32_t page_table_size = PAGE_SIZE * 256;                     // 这个是已经分配了的页表的大小
    uint32_t kernel_pool_start_addr = page_table_size + 0x00100000; //内核的pool的开始地址

    //剩余可用的
    uint32_t free_mem = all_mem - kernel_pool_start_addr;

    uint32_t free_mem_page = free_mem / PAGE_SIZE;

    uint16_t kernel_mem_page = free_mem_page / 2;

    uint16_t user_mem_page = free_mem_page - kernel_mem_page;

    uint32_t user_pool_start_addr = kernel_pool_start_addr + kernel_mem_page * PAGE_SIZE;

    /*设置内核内存池的属性 */
    kernel_pool.phy_start_addr = kernel_pool_start_addr;
    kernel_pool.mem_size = kernel_mem_page * PAGE_SIZE;
    kernel_pool.pool_bitmap.bitmap_len = kernel_mem_page / 8;

    /*设置用户内存池属性 */
    user_pool.phy_start_addr = user_pool_start_addr;
    user_pool.mem_size = user_mem_page * PAGE_SIZE;
    user_pool.pool_bitmap.bitmap_len = user_mem_page / 8;

    kernel_pool.pool_bitmap.bits = (void *)KERNEL_BITMAP_START;
    user_pool.pool_bitmap.bits = (void *)(KERNEL_BITMAP_START + kernel_pool.pool_bitmap.bitmap_len);

    //下面是一些输出信息
    putStr("    kernel_pool_bitmap_start: ");
    putInt((uint32_t)kernel_pool.pool_bitmap.bits);
    putStr("    kernel_pool_phy_start: ");
    putInt((uint32_t)kernel_pool.phy_start_addr);
    putChar('\n');
    putStr("    user_pool_bitmap_start:   ");
    putInt((uint32_t)user_pool.pool_bitmap.bits);
    putStr("    user_pool_phy_start: ");
    putInt((uint32_t)user_pool.phy_start_addr);
    putChar('\n');

    /*位图初始化,设置为0 */
    bitmap_init(&(kernel_pool.pool_bitmap));
    bitmap_init(&(user_pool.pool_bitmap));

    /*设置kernel_vaddr */
    kernel_vaddr.vaddr_bitmap.bits = user_pool.pool_bitmap.bits + user_pool.pool_bitmap.bitmap_len;
    kernel_vaddr.vaddr_bitmap.bitmap_len = kernel_pool.pool_bitmap.bitmap_len;
    kernel_vaddr.vaddr_start = KERNEL_HEAP_START;

    bitmap_init(&(kernel_vaddr.vaddr_bitmap));
    putStr("mem pool init done\n");
}

void mem_init()
{
    putStr("mem init start\n");
    uint32_t memory_size = *((uint32_t *)(0xb00));
    mem_pool_init(memory_size);
    putStr("mem init done\n");
}
