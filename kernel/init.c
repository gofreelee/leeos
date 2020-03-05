#include "init.h"
#include "print.h"
#include "interrupt.h"

void init_all()
{
    putStr("init all start \n");
    idt_init();
}
