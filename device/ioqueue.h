#ifndef IOQUEUE_H_
#define IOQUEUE_H_
#include "../thread/thread.h"
#include "../thread/sync.h"

#define buf_size 64
struct ioqueue
{
    struct lock io_lock;
    struct pcb_strcut *producer;
    struct pcb_struct *consumer;
    char buf[buf_size]; // 缓冲区
    int32_t head;
    int32_t tail;
};
void ioqueue_init(struct ioqueue *queue);
bool ioqueue_is_full(struct ioqueue *queue);
bool ioqueue_is_empty(struct ioqueue *queue);
char ioq_getchar(struct ioqueue *ioq);
void ioq_putchar(struct ioqueue *ioq, char byte);
#endif