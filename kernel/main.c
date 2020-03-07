#include "../lib/kernel/print.h"
#include "init.h"
#include "debug.h"
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
    init_all();
    asm volatile("sti");
    ASSERT(1 == 2)
    while (1)
        ;
    return 0;
}