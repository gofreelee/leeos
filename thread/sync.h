#ifndef SYNC_H_
#define SYNC_H_
#include "../lib/kernel/list.h"
#include "../lib/stdint.h"
#include "thread.h"

struct semaphore
{
    uint8_t value;
    struct list waiter;
};

struct lock
{
    struct pcb_struct *holder;
    struct semaphore sem;
    uint32_t holder_repeat_num;
};

void sem_down(struct semaphore *sem);
void sem_up(struct semaphore *sem);
void lock_acquire(struct lock *plock);
void lock_release(struct lock *plock);
void sem_init(struct semaphore *sem, uint8_t value);
void lock_init(struct lock *plock);
#endif
