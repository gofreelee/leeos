#include "console.h"
#include "../thread/sync.h"
#include "../thread/thread.h"
#include "../lib/kernel/print.h"
static struct lock console_lock;
void console_init()
{
    //初始化锁
    lock_init(&console_lock);
}

void console_acquire()
{
    lock_acquire(&console_lock);
}

void console_release()
{
    lock_release(&console_lock);
}

void console_put_str(char *str)
{
    console_acquire();
    putStr(str);
    console_release();
}

void console_putChar(uint8_t ch)
{
    console_acquire();
    putStr(ch);
    console_release();
}

void console_put_int(uint32_t num)
{
    console_acquire();
    putInt(num);
    console_release();
}