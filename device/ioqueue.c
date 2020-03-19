#include "ioqueue.h"
#include "../kernel/interrupt.h"
#include "../kernel/debug.h"
void ioqueue_init(struct ioqueue *queue)
{
    lock_init(&(queue->io_lock));
    queue->consumer = queue->producer = 0;
    queue->head = queue->tail = 0;
}

static int32_t next_pos(uint32_t pos)
{
    return (pos + 1) % buf_size;
}

bool ioqueue_is_full(struct ioqueue *queue)
{
    ASSERT(get_intr_status() == INTR_OFF)
    return next_pos(queue->head) == queue->tail;
}

bool ioqueue_is_empty(struct ioqueue *queue)
{
    return queue->head == queue->tail;
}

static void ioq_wait(struct pcb_struct **waiter)
{
    ASSERT(waiter != 0 && (*waiter == 0))
    *waiter = running_thread();
    thread_lock(TASK_BLOCKED);
}

static void ioq_wakeup(struct pcb_struct **waiter)
{
    ASSERT(*waiter != 0)
    thread_unlock(*waiter);
    *waiter = 0;
}

char ioq_getchar(struct ioqueue *ioq)
{
    ASSERT(get_intr_status() == INTR_OFF)
    while (ioqueue_is_empty(ioq))
    {
        lock_acquire(&(ioq->io_lock));
        ioq_wait(&ioq->consumer);
        lock_release(&(ioq->io_lock));
    }
    char byte = ioq->buf[ioq->tail];
    ioq->tail = next_pos(ioq->tail);
    if (ioq->producer != 0)
    {
        ioq_wakeup(ioq->producer);
    }
    return byte;
}

void ioq_putchar(struct ioqueue *ioq, char byte)
{
    ASSERT(get_intr_status() == INTR_OFF)
    while (ioqueue_is_full(ioq))
    {
        lock_acquire(&(ioq->io_lock));
        ioq_wait(&(ioq->producer));
        lock_release(&(ioq->io_lock));
    }
    ioq->buf[ioq->head] = byte;
    ioq->head = next_pos(ioq->head);
    if (ioq->consumer != 0)
    {
        ioq_wakeup(ioq->consumer);
    }
    return;
}
