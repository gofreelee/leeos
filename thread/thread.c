#include "thread.h"
#include "../kernel/memory.h"
#include "../lib/string.h"
#include "../kernel/debug.h"
#include "../lib/kernel/list.h"
#include "../kernel/interrupt.h"
#include "../lib/kernel/print.h"
#define offset(struct_type, elem_name) \
    (uint32_t)(&((struct_type *)0)->elem_name)

#define get_pcb(struct_type, elem_name, curr_addr) \
    (struct_type *)((uint32_t)curr_addr - offset(struct_type, elem_name))

struct list ready_thread_list;
struct list all_thread_list;

struct pcb_struct *main_thread_pcb_ptr;

struct list_elem *thread_tag; //

extern switch_to(struct pcb_struct *curr, struct pcb_struct *next);

struct pcb_struct *running_thread()
{
    /*获取正在执行的线程的pcb的地址 */
    uint32_t pcb;
    asm volatile("movl %%esp, %0"
                 : "=g"(pcb));
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
    memset(pcb_ptr, 0, sizeof(*pcb_ptr)); //初始化

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

void schedule()
{
    /*保证中断关了 */
    ASSERT(get_intr_status() == INTR_OFF)
    struct pcb_struct *curr = running_thread();
    if (curr->status == TASK_RUNNING)
    {
        /*时间片到了 */
        ASSERT(!elem_find(&ready_thread_list, &curr->general_tag));
        curr->ticks = curr->priority;
        curr->status = TASK_READY;
        list_append(&ready_thread_list, &(curr->general_tag));
    }
    else
    {
        /*其他运行态 */
    }
    //确保有就绪状态的线程
    ASSERT(!list_empty(&ready_thread_list))
    thread_tag = list_pop(&ready_thread_list);
    struct pcb_struct *next =
        get_pcb(struct pcb_struct, general_tag, thread_tag);
    next->status = TASK_RUNNING;
    switch_to(curr, next);
}

void system_thread_init()
{
    putStr("thread_init start\n");
    list_init(&all_thread_list);
    list_init(&ready_thread_list);
    make_main_thread();
    putStr("thread_init done\n");
}