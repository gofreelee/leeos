#include "../lib/kernel/print.h"
#include "init.h"
#include "debug.h"
#include "memory.h"
#include "../thread/thread.h"
#include "../kernel/interrupt.h"

void thread1(void *arg);
void thread2(void *arg);
int main()
{
    // putChar('k');
    // putChar('e');
    // putChar('r');
    // putChar('n');
    // putChar('e');
    // putChar('l');
    // putChar('\n');
    // putChar('1');
    // putChar('2');
    // putChar('\b');
    // putChar('3');
    putStr("kernel\n");
    mem_init();
    init_all();
    void *addr = malloc_kernel_pages(3);
    putStr("\n malloc kernel pages start vaddr is");
    putInt((uint32_t)addr);
    putChar('\n');
    intr_close();

    thread_start(thread1, "kthread", "thread1", 31);

    thread_start(thread2, "thread2", "thread2", 8);
    asm volatile("sti");
    while (1)
    {
        putStr("main");
    }
    return 0;
}
void thread1(void *arg)
{
    char *args = arg;
    while (1)
    {
        putStr(args);
        //putChar('\n');
    }
}
void thread2(void *arg)
{
    char *args = arg;
    while (1)
    {
        putStr(args);
        //putChar('\n');
    }
}