#include "thread.h"
#include "../kernel/memory.h"
#include "../lib/string.h"

void kernel_thread(thread_func *function, void *func_args)
{
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

void thread_init(struct pcb_struct *pcb_ptr, int prio)
{
    pcb_ptr->priority = 0;
    pcb_ptr->stack_magic = 0x00007c00; // 魔数
    pcb_ptr->status = TASK_RUNNING;
    pcb_ptr->self_kernel_stack = (uint32_t *)((uint32_t)pcb_ptr + 4096);
    /*预留中断栈 */
    pcb_ptr->self_kernel_stack -= sizeof(struct intr_stack);
    /*预留线程栈 */
    pcb_ptr->self_kernel_stack -= sizeof(struct thread_stack);
}

void thread_start(thread_func func, void *func_args,
                  const char *name, int prio)
{
    /*先分配整页的内存 */
    struct pcb_struct *pcb_ptr = malloc_kernel_pages(1);
    memset(pcb_ptr, 0, 4096); //初始化
    strcpy(pcb_ptr->name, name);
    thread_init(pcb_ptr, prio);
    thread_create(pcb_ptr, func, func_args);
    asm volatile("mov %0, %%esp;pop %%ebp;pop %%ebx; pop %%edi;pop %%esi; ret "
                 :
                 : "g"(pcb_ptr->self_kernel_stack));
}