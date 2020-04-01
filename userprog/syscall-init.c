#include "syscall-init.h"
#include "../thread/thread.h"
#include "../lib/stdint.h"
#include "../lib/user/syscall.h"
#include "../lib/kernel/print.h"
#include "../device/console.h"
#include "../lib/string.h"
//32个系统调用
#define SYSTEMCALL_NUMBER 32
// 系统调用处理函数
void *syscall_table[SYSTEMCALL_NUMBER];
uint32_t sys_get_pid()
{
    return (uint32_t)(running_thread()->pid);
}

uint32_t sys_write(const char *str)
{
    console_put_str(str);
    return strlen(str);
}

void syscall_table_init()
{
    putStr("syscall table init start \n");
    syscall_table[SYS_GETPID] = sys_get_pid;
    syscall_table[SYS_WRITE] = sys_write;
    putStr("syscall table init end \n");
}