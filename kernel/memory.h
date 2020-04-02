#ifndef MEMORY_H_
#define MEMORY_H_
#include "../lib/bitmap.h"
#include "../lib/stdint.h"
#include "../lib/kernel/list.h"
#define PG_P_1 1
#define PG_P_0 0
#define PG_RW_R 0
#define PG_RW_W 2
#define PG_US_U 4
#define PG_US_S 0
#define PAGE_SIZE 4096
#define MEM_BLOCK_DES_INT 7 //共七类描述符 16 32 64 128 256 512 1024

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

/*内存块 */
struct mem_block
{
    struct list_elem free_elem;
};

/*内存块描述符*/
struct mem_block_desc
{
    uint32_t block_size;       // 此类内存存放的内存块大小
    uint32_t blocks_per_arena; //该arena 能存放的block数量
    struct list free_list;
};

struct arena
{
    struct mem_block_desc *mem_desc_addr; // 内存块描述符的地址
    uint32_t cnt;
    bool large; // 当large 为　true cnt表示可用的内存块的数量
    //当large 为false 表示的是所占用的页框数．
};

extern struct pool kernel_pool, user_pool;
void mem_init();
void *malloc_pages(enum pool_flags PF, uint32_t cnt_pages);
static void *vaddr_get_pages(enum pool_flags PF, uint32_t cnt);
void *malloc_kernel_pages(uint32_t cnt);
uint32_t addr_virtual_to_phy(uint32_t vaddr);
void *get_a_page(enum pool_flags PF, uint32_t vaddr);
void *malloc_user_pages(uint32_t cnt);
void block_des_init(struct mem_block_desc *mem_block_descs);
void *sys_malloc(uint32_t size);
#endif