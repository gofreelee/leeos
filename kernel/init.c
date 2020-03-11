#include "init.h"
#include "print.h"
#include "interrupt.h"
#include "../thread/thread.h"
#include "../device/timer.h"
void init_all()

{
    putStr("init all start \n");
    idt_init();
    system_thread_init();
    timer_init();
}
