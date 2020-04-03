#include "../thread/thread.h"
#include "../kernel/global.h"
#include "../kernel/memory.h"
#include "../lib/kernel/print.h"
#include "../lib/string.h"
#include "tss.h"
#include "../kernel/interrupt.h"
#include "../kernel/debug.h"
#include "process.h"

extern void exit_intr();

void start_process(void *filename)
{
    void *function = filename;
    struct pcb_struct *pcb = running_thread();
    struct intr_stack *intr_stack =
        pcb->self_kernel_stack + sizeof(struct thread_stack);
    intr_stack->eax = intr_stack->ebx = intr_stack->ecx = intr_stack->edx = 0;
    intr_stack->ebp = intr_stack->edi = intr_stack->esi = intr_stack->esp_dummy = 0;
    intr_stack->cs = SELECTOR_U_CODE;
    intr_stack->ds = intr_stack->fs = SELECTOR_U_DATA;
    intr_stack->gs = 0;
    intr_stack->eip = function;
    intr_stack->es = intr_stack->ss = SELECTOR_U_DATA;
    intr_stack->eflags = (EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1);
    intr_stack->esp = (void *)((uint32_t)get_a_page(PF_USER, USER_STACK3_VADDR) + PAGE_SIZE);
    asm volatile("movl %0,%%esp; add $4,%%esp;jmp exit_intr"
                 :
                 : "g"(intr_stack)
                 : "memory");
}

/*更新页表 */
void page_dir_activate(struct pcb_struct *pthread)
{
    uint32_t pg_dir = 0x00100000;
    if (pthread->pgdir != 0)
    {
        pg_dir = addr_virtual_to_phy((uint32_t)pthread->pgdir);
    }
    asm volatile("movl %0, %%cr3"
                 :
                 : "r"(pg_dir)
                 : "memory");
}

void process_activate(struct pcb_struct *pcb)
{
    /*这个函数用来开启对应的线程 */
    if (pcb == 0)
    {
        putStr("process_activate error \n");
    }

    page_dir_activate(pcb);

    if (pcb->pgdir != 0)
    {
        update_tss_esp0(pcb);
    }
}

uint32_t *create_page_dir()
{
    /*从内核内存池中分配一页内存 */
    uint32_t *page_dir_vaddr = malloc_kernel_pages(1);
    if (page_dir_vaddr == 0)
    {
        putStr("create　page dir error ! \n");
        return 0;
    }

    /*把内核页目录的769 - 1024拷贝到新的页表项 */
    memcpy((void *)((uint32_t)page_dir_vaddr + 0x300 * 4),
           (void *)(0xfffff000 + 0x300 * 4),
           1024);

    /*把新的页目录表的最后一项改成自己本身的物理地址*/
    page_dir_vaddr[1023] = addr_virtual_to_phy((uint32_t)page_dir_vaddr) | PG_US_U | PG_RW_W | PG_P_1;
    return page_dir_vaddr;
}

/*创建用户进程的虚拟地址管理位图 */
void create_user_vaddr_bitmap(struct pcb_struct *user_prog)
{
    user_prog->userprog_vaddr.vaddr_start = USER_PROG_START_VADDR;
    uint32_t bit_cnts =
        DIV_ROUND_UP((0xc0000000 - USER_PROG_START_VADDR) / PAGE_SIZE / 8, PAGE_SIZE);
    user_prog->userprog_vaddr.vaddr_bitmap.bitmap_len = (0xc0000000 - USER_PROG_START_VADDR) / PAGE_SIZE / 8;
    user_prog->userprog_vaddr.vaddr_bitmap.bits = malloc_kernel_pages(bit_cnts);
    //putInt(user_prog->userprog_vaddr.vaddr_bitmap.bits);
    bitmap_init(&(user_prog->userprog_vaddr.vaddr_bitmap));
}

void process_execute(void *filename, char *name)
{
    struct pcb_struct *process = malloc_kernel_pages(1);
    if (process == 0)
    {
        putStr("malloc_kernel_pages error");
    }
    thread_init(process, 31, name); // 31是默认的优先级

    create_user_vaddr_bitmap(process);

    thread_create(process, start_process, filename);

    process->pgdir = create_page_dir();

    block_des_init(process->u_mem_block_desc);

    enum intr_status old_status = intr_disable();

    ASSERT(!elem_find(&ready_thread_list, &(process->general_tag)));
    list_append(&ready_thread_list, &(process->general_tag));
    ASSERT(!elem_find(&all_thread_list, &(process->all_list_tag)));
    list_append(&all_thread_list, &(process->all_list_tag));

    intr_set_status(old_status);
}