#include "memory.h"
#include "../lib/kernel/print.h"
#include "../lib/stdint.h"
#include "debug.h"
#include "../lib/string.h"
#include "../thread/sync.h"
#include "../thread/thread.h"

#define PAGE_SIZE 4096 // 一个页的大小为 4KB

#define KERNEL_HEAP_START 0xc0100000 // 我们设置的内核的堆的起始地址,跨过低端1MB

#define KERNEL_BITMAP_START 0xc009a000
/*内存池结构 */
struct pool
{
    struct bitmap pool_bitmap;
    uint32_t *phy_start_addr;
    uint32_t mem_size; // 内存池长度
    struct lock lock;  // 锁
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
    lock_init(&(kernel_pool.lock));
    /*设置用户内存池属性 */
    user_pool.phy_start_addr = user_pool_start_addr;
    user_pool.mem_size = user_mem_page * PAGE_SIZE;
    user_pool.pool_bitmap.bitmap_len = user_mem_page / 8;
    lock_init(&(user_pool.lock));
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

static void *vaddr_get_pages(enum pool_flags PF, uint32_t page_cnt)
{
    uint32_t bitstart = -1, counter = 0, alloced_pages_addr;
    if (PF == PF_KERNEL)
    {
        bitstart = bitmap_scan(&(kernel_vaddr.vaddr_bitmap), page_cnt);
        if (bitstart == -1)
        {
            return 0;
        }
        else
        {
            while (counter < page_cnt)
            {
                bitmap_set(&(kernel_vaddr.vaddr_bitmap), bitstart + counter, 1);
                ++counter;
            }
            alloced_pages_addr = kernel_vaddr.vaddr_start + PAGE_SIZE * bitstart;
        }
    }
    else
    {
        /*用户进程分配 */
        struct pcb_struct *curr = running_thread();
        bitstart = bitmap_scan(&(curr->userprog_vaddr.vaddr_bitmap), page_cnt);
        if (bitstart == -1)
        {
            return 0;
        }
        else
        {
            int counter = 0;
            while (counter < page_cnt)
            {
                bitmap_set(&(curr->userprog_vaddr.vaddr_bitmap), bitstart + counter, 1);
                ++counter;
            }
            alloced_pages_addr = curr->userprog_vaddr.vaddr_start + PAGE_SIZE * bitstart;
            /*(0xc0000000 - PAGE_SIZE) 作为用户３级栈已经在start_process中分配 */
            ASSERT((uint32_t)alloced_pages_addr < (0xc0000000 - PAGE_SIZE))
        }
    }
    return (void *)alloced_pages_addr;
}

static uint32_t *pte_addr(uint32_t ptr)
{
    /*该函数的作用是,把ptr(一个地址), 这个地址是属于一个页的,页是由页表的页表项
    索引得的,然后把这个页表的虚拟地址返回 */
    uint32_t pte_vaddr = (0xffc00000) |
                         ((ptr & 0xffc00000) >> 10) |
                         4 * ((ptr & 0x003ff000) >> 12);
    return (uint32_t *)pte_vaddr;
}

static uint32_t *pde_addr(uint32_t ptr)
{
    uint32_t pde_vaddr = (0xfffff000 | ((ptr & 0xffc00000) >> 22) * 4);
    return (uint32_t *)pde_vaddr;
}

static void *palloc(struct pool *mem_pool)
{
    /*在这个内存池中分配一页的内存 */

    uint32_t bit_index = bitmap_scan(&(mem_pool->pool_bitmap), 1);
    if (bit_index == -1)
    {
        /*分配失败 */
        return 0;
    }
    //putInt(bit_index);
    bitmap_set(&(mem_pool->pool_bitmap), bit_index, 1);

    uint32_t palloc_addr = mem_pool->phy_start_addr + PAGE_SIZE * bit_index;
    return (void *)palloc_addr;
}

static void page_table_add(void *vir_addr, void *phy_addr)
{
    /*先把虚拟地址对应的页目录项的指针，页表项的指针 */
    uint32_t *pte_ptr = pte_addr(vir_addr);
    uint32_t *pde_ptr = pde_addr(vir_addr);

    /*页目录项存在 */
    if ((*pde_ptr & 0x00000001))
    {
        /*先判断 */
        ASSERT(!(*pte_ptr & 0x00000001))
        if (!(*pte_ptr & 0x00000001))
        {
            *pte_ptr = ((uint32_t)phy_addr | PG_P_1 | PG_RW_W | PG_US_U);
        }
        else
        {

            /*页表项已经存在的情况下 */
        }
    }
    /*页目录项不存在 */
    else
    {
        /*分配一个页目录空间 */
        uint32_t *alloc_pde = (uint32_t *)palloc(&kernel_pool);
        *pde_ptr = ((uint32_t)alloc_pde | PG_P_1 | PG_RW_W | PG_US_U);
        memset((void *)((uint32_t)pte_ptr & 0xfffff000), 0, PAGE_SIZE);
        ASSERT(!(*pte_ptr & 0x00000001))
        *pte_ptr = ((uint32_t)phy_addr | PG_P_1 | PG_RW_W | PG_US_U);
    }
}

void *malloc_pages(enum pool_flags PF, uint32_t cnt_pages)
{
    /*分配虚拟地址和物理地址，并建立它们的映射 */
    ASSERT(cnt_pages > 0 && cnt_pages < 3840)
    void *keep_help;
    void *vir_alloc_addr = vaddr_get_pages(PF, cnt_pages);
    /*错误判断 */
    if (vir_alloc_addr == 0)
        return 0;

    keep_help = vir_alloc_addr;
    void *phy_alloc_addr;
    struct pool *mem_pool = (PF == PF_KERNEL ? &kernel_pool : &user_pool);

    for (int i = 0; i < cnt_pages; ++i)
    {
        //debug 这里

        phy_alloc_addr = palloc(mem_pool);

        if (phy_alloc_addr == 0)
            return 0;

        page_table_add(vir_alloc_addr, phy_alloc_addr);
        vir_alloc_addr = (void *)((uint32_t)vir_alloc_addr + PAGE_SIZE);
    }
    return keep_help;
}

void *malloc_kernel_pages(uint32_t cnt)
{
    void *vaddr = malloc_pages(PF_KERNEL, cnt);
    if (vaddr != 0)
    {
        memset(vaddr, 0, PAGE_SIZE * cnt);
    }
    return vaddr;
}

void *malloc_user_pages(uint32_t cnt)
{
    /*分配用户内存 */
    lock_acquire(&(user_pool.lock));
    void *vaddr = malloc_pages(PF_USER, cnt);
    memset(vaddr, 0, cnt);
    lock_release(&(user_pool.lock));
    return vaddr;
}

void *get_a_page(enum pool_flags PF, uint32_t vaddr)
{
    struct pool *mem_pool = (PF == PF_KERNEL) ? &kernel_pool : &user_pool;
    lock_acquire(&(mem_pool->lock));
    struct pcb_struct *curr = running_thread();
    int bit_index = -1;
    if (curr->pgdir != 0 && PF == PF_USER)
    {
        /*用户进程分配内存 */
        bit_index = (vaddr - (uint32_t)curr->userprog_vaddr.vaddr_start) / PAGE_SIZE;
        ASSERT(bit_index > 0)
        bitmap_set(&curr->userprog_vaddr.vaddr_bitmap, bit_index, 1);
    }
    else if (curr->pgdir == 0 && PF == PF_KERNEL)
    {
        bit_index = (vaddr - (uint32_t)kernel_vaddr.vaddr_start) / PAGE_SIZE;
        ASSERT(bit_index > 0)
        bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_index, 1);
    }
    else
    {
        /*报告下错误 */
    }
    void *phy_addr = palloc(mem_pool);
    page_table_add((void *)vaddr, phy_addr);
    lock_release(&(mem_pool->lock));
    return (void *)vaddr;
}

/*由虚拟地址获得对应的物理地址 */
uint32_t addr_virtual_to_phy(uint32_t vaddr)
{
    uint32_t *pte_ptr = pte_addr(vaddr);
    return ((*pte_ptr) & 0xfffff000) + (vaddr & 0x00000fff);
}