#include "sync.h"
#include "../kernel/debug.h"
#include "../kernel/interrupt.h"
#include "../lib/kernel/print.h"
void sem_init(struct semaphore *sem, uint8_t value)
{
    sem->value = value;
    list_init(&(sem->waiter));
}

void lock_init(struct lock *plock)
{
    //信号量初始为１
    sem_init(&(plock->sem), 1);
    plock->holder = 0;
    plock->holder_repeat_num = 0;
}

void sem_down(struct semaphore *sem)
{
    enum task_status old_status = intr_disable();
    while (sem->value == 0)
    {
        if (elem_find(&sem->waiter, &(running_thread()->general_tag)))
        {
            EXCEPTION_REPORT("sem is in waiter\n");
        }
        list_append(&sem->waiter, &(running_thread()->general_tag)); //加入等待队列
        thread_lock(TASK_BLOCKED);                                   //自己主动堵塞
    }
    --(sem->value); //
    ASSERT(sem->value == 0)
    intr_set_status(old_status);
}

void sem_up(struct semaphore *sem)
{
    enum task_status old_status = intr_disable();
    ASSERT(sem->value == 0)

    if (!list_empty(&sem->waiter))
    {
        //等待队列不空，放一个线程出来.
        struct pcb_struct *popthread =
            get_pcb(struct pcb_struct, general_tag, list_pop(&sem->waiter));
        // putChar('(');
        // putInt((uint32_t)popthread);
        // putChar(')');

        thread_unlock(popthread);
    }
    ++(sem->value);
    ASSERT(sem->value == 1)
    intr_set_status(old_status);
}

void lock_acquire(struct lock *plock)
{
    if (plock->holder != running_thread())
    {
        //如果自己已经持有锁,避免二次获取
        sem_down(&(plock->sem));
        plock->holder = running_thread();
        ASSERT(plock->holder_repeat_num == 0);
        plock->holder_repeat_num = 1;
    }
    else
    {
        //重复holder
        ++(plock->holder_repeat_num);
    }
}

void lock_release(struct lock *plock)
{

    ASSERT(plock->holder == running_thread())
    if (plock->holder_repeat_num > 1)
    {
        --(plock->holder_repeat_num);
        return;
    }
    --(plock->holder_repeat_num);
    plock->holder = 0;
    plock->holder_repeat_num = 0;
    sem_up(&plock->sem); //唤醒一个线程
}
