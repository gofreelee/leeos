#include "init.h"
#include "print.h"
#include "interrupt.h"
#include "../thread/thread.h"
#include "../device/timer.h"
#include "../device/console.h"
#include "../device/keyboard.h"
#include "../userprog/tss.h"
#include "../userprog/syscall-init.h"
void init_all()

{
    putStr("init all start \n");
    idt_init();
    mem_init();
    system_thread_init();
    timer_init();
    console_init();
    keyboard_init();
    tss_init();
    syscall_table_init();
}
