#include "thread.h"
#include "../kernel/memory.h"
#include "../lib/string.h"
#include "../kernel/debug.h"
#include "../lib/kernel/list.h"
#include "../kernel/interrupt.h"
#include "../lib/kernel/print.h"
#include "../userprog/process.h"
#include "sync.h"
struct list ready_thread_list;
struct list all_thread_list;
struct lock pid_lock; // 线程号锁
struct pcb_struct *main_thread_pcb_ptr;
struct list_elem *thread_tag; //

static struct pcb_struct *idle_thread;

static pid_t next_pid = 0;

extern void switch_to(struct pcb_struct *curr, struct pcb_struct *next);

struct pcb_struct *running_thread()
{
    /*获取正在执行的线程的pcb的地址 */
    uint32_t pcb;
    asm volatile("movl %%esp, %0"
                 : "=g"(pcb));
    return (struct pcb_struct *)(pcb & 0xfffff000);
}

void kernel_thread(thread_func *function, void *func_args)
{
    intr_enable();
    function(func_args);
}

void thread_create(struct pcb_struct *pcb_ptr,
                   thread_func function, void *func_args)
{
    /*预留中断栈 */
    pcb_ptr->self_kernel_stack -= sizeof(struct intr_stack);
    /*预留线程栈 */
    pcb_ptr->self_kernel_stack -= sizeof(struct thread_stack);

    struct thread_stack *thread_stack_ptr = pcb_ptr->self_kernel_stack;
    thread_stack_ptr->eip = kernel_thread;
    thread_stack_ptr->function = function;
    thread_stack_ptr->function_args = func_args;
    thread_stack_ptr->ebp = thread_stack_ptr->ebx = thread_stack_ptr->edi = thread_stack_ptr->esi = 0;
}

void thread_init(struct pcb_struct *pcb_ptr, int prio, char *name)
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
    pcb_ptr->pid = allocate_next_pid();
    pcb_ptr->priority = prio;
    pcb_ptr->elapsed_ticks = 0;
    pcb_ptr->ticks = prio;
    pcb_ptr->pgdir = 0;
    pcb_ptr->fd_table[0] = 0;
    pcb_ptr->fd_table[1] = 1;
    pcb_ptr->fd_table[2] = 2;
    for (int i = 3; i < MAX_FILES_OPEN_PER_PROC; ++i)
    {
        pcb_ptr->fd_table[i] = -1;
    }
    pcb_ptr->stack_magic = 0x00007c00; // 魔数
    pcb_ptr->self_kernel_stack = (uint32_t *)((uint32_t)pcb_ptr + 4096);
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
    // uint32_t num = list_len(&ready_thread_list);
    // putInt(num);
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

    ASSERT(intr_get_status() == INTR_OFF)
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
    if (list_empty(&ready_thread_list))
    {
        thread_unlock(idle_thread);
    }
    ASSERT(!list_empty(&ready_thread_list))
    thread_tag = (void *)0;
    thread_tag = list_pop(&ready_thread_list);
    struct pcb_struct *next =
        get_pcb(struct pcb_struct, general_tag, thread_tag);
    next->status = TASK_RUNNING;
    //putStr("\n schedule \n");

    process_activate(next);

    switch_to(curr, next);
    //putStr("\n scheduled \n");
    // return;
}

void system_thread_init()
{
    putStr("thread_init start\n");
    list_init(&all_thread_list);
    list_init(&ready_thread_list);
    lock_init(&pid_lock);
    make_main_thread();
    idle_thread = thread_start(idle, 0, "idle", 10);
    putStr("thread_init done\n");
}

void thread_lock(enum task_status status)
{
    //思路，这个函数是把当前的线程置于一个不可运行的状态，
    //这个状态本身就作为参数传入了，就是status;
    enum task_status old_status = intr_disable(); // 要关上中断，防止时钟中断影响
    ASSERT(status == TASK_BLOCKED || status == TASK_WAITING || status == TASK_HANGING);
    struct pcb_struct *curr_ptr = running_thread();
    curr_ptr->status = status; // 设置当前线程的状态切换.
    //下面执行调度
    schedule();
    //执行流又回到本线程中后再执行这个开启中断
    intr_set_status(old_status);
}

void thread_unlock(struct pcb_struct *unlock_thread)
{
    enum task_status old_status = intr_disable();
    enum task_status status = unlock_thread->status;
    ASSERT(status == TASK_BLOCKED || status == TASK_WAITING || status == TASK_HANGING);
    if (unlock_thread->status != TASK_READY)
    {
        ASSERT(!elem_find(&ready_thread_list, &(unlock_thread->general_tag)));
        if (elem_find(&ready_thread_list, &(unlock_thread->general_tag)))
        {
            EXCEPTION_REPORT("blocked list is ready list\n");
        }
        unlock_thread->status = TASK_READY;
        list_push(&ready_thread_list, &(unlock_thread->general_tag));
    }
    intr_set_status(old_status);
}

pid_t allocate_next_pid()
{
    lock_acquire(&pid_lock);
    next_pid++;
    lock_release(&pid_lock);
    return next_pid;
}

static void idle(void *arg UNUSED)
{
    while (1)
    {
        thread_lock(TASK_BLOCKED);
        asm volatile("sti;hlt" ::
                         : "memory");
    }
}

void thread_yield()
{
    struct pcb_struct *curr_thread = running_thread();
    enum intr_status old_status = intr_disable();
    ASSERT(!elem_find(&ready_thread_list, curr_thread));
    list_append(&ready_thread_list, &curr_thread->general_tag);
    curr_thread->status = TASK_READY;
    schedule();
    intr_set_status(old_status);
}