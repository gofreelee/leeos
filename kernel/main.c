#include "../lib/kernel/print.h"
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
    putInt(0x00000000);
    putChar('\n');
    putInt(0xffffffff);
    while (1);
    return 0;
}