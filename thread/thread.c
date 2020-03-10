#include "thread.h"
#include "../kernel/memory.h"
#include "../lib/string.h"
#include "../kernel/debug.h"
#include "../lib/kernel/list.h"
#include "../kernel/interrupt.h"
struct list ready_thread_list;
struct list all_thread_list;

struct pcb_struct *main_thread_pcb_ptr;

struct pcb_struct *running_thread()
{
    /*获取正在执行的线程的pcb的地址 */
    uint32_t pcb;
    asm volatile("mov %%esp, %0"
                 : "g"(pcb));
    return (pcb & 0xfffff000);
}

void kernel_thread(thread_func *function, void *func_args)
{
    intr_open();
    function(func_args);
}

void thread_create(struct pcb_struct *pcb_ptr,
                   thread_func function, void *func_args)
{
    struct thread_stack *thread_stack_ptr = pcb_ptr->self_kernel_stack;
    thread_stack_ptr->eip = kernel_thread;
    thread_stack_ptr->function = function;
    thread_stack_ptr->function_args = func_args;
    thread_stack_ptr->ebp = thread_stack_ptr->ebx = thread_stack_ptr->edi = thread_stack_ptr->esi = 0;
}

void thread_init(struct pcb_struct *pcb_ptr, int prio, const char *name)
{
    memset(pcb_ptr, 0, 4096); //初始化
    strcpy(pcb_ptr->name, name);
    if (pcb_ptr == main_thread_pcb_ptr)
    {
        pcb_ptr->status = TASK_RUNNING;
    }
    else
    {
        pcb_ptr->status = TASK_READY;
    }

    pcb_ptr->priority = prio;
    pcb_ptr->elapsed_ticks = 0;
    pcb_ptr->ticks = prio;
    pcb_ptr->pgdir = 0;
    pcb_ptr->stack_magic = 0x00007c00; // 魔数
    pcb_ptr->self_kernel_stack = (uint32_t *)((uint32_t)pcb_ptr + 4096);
    /*预留中断栈 */
    pcb_ptr->self_kernel_stack -= sizeof(struct intr_stack);
    /*预留线程栈 */
    pcb_ptr->self_kernel_stack -= sizeof(struct thread_stack);
}

struct pcb_struct *thread_start(thread_func func, void *func_args,
                                const char *name, int prio)
{
    /*先分配整页的内存 */
    struct pcb_struct *pcb_ptr = malloc_kernel_pages(1);

    thread_init(pcb_ptr, prio, name);
    thread_create(pcb_ptr, func, func_args);
    // asm volatile("mov %0, %%esp;pop %%ebp;pop %%ebx; pop %%edi;pop %%esi; ret "
    //              :
    //              : "g"(pcb_ptr->self_kernel_stack));
    /*加入就绪队列 */
    ASSERT(!(elem_find(&ready_thread_list, &(pcb_ptr->general_tag))));
    list_append(&ready_thread_list, &(pcb_ptr->general_tag));

    /*加入全部队列 */
    ASSERT(!(elem_find(&all_thread_list, &(pcb_ptr->all_list_tag))));
    list_append(&all_thread_list, &(pcb_ptr->all_list_tag));
    return pcb_ptr;
}

static void make_main_thread()
{
    main_thread_pcb_ptr = running_thread();
    thread_init(main_thread_pcb_ptr, 31, "main");
    ASSERT(!(elem_find(&all_thread_list, &(main_thread_pcb_ptr->all_list_tag))))
    list_append(&all_thread_list, &(main_thread_pcb_ptr->all_list_tag));
}