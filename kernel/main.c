#include "../lib/kernel/print.h"
#include "init.h"
#include "debug.h"
#include "memory.h"
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
    asm volatile("sti");
    void *addr = malloc_kernel_pages(3);
    putStr("\n malloc kernel pages start vaddr is");
    putInt((uint32_t)addr);
    putChar('\n');
    ASSERT(1 == 2)
    while (1)
        ;
    return 0;
}