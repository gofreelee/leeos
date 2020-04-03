#include "../lib/kernel/print.h"
#include "init.h"
#include "debug.h"
#include "memory.h"
#include "../thread/thread.h"
#include "../kernel/interrupt.h"
#include "../device/console.h"
#include "../device/keyboard.h"
#include "../device/ioqueue.h"
#include "../userprog/process.h"
#include "../userprog/syscall-init.h"
#include "../lib/user/syscall.h"
#include "../lib/stdio.h"

void thread1(void *arg);
void thread2(void *arg);
void u_prog_a();
void u_prog_b();

static int prog_a_pid = 0, prog_b_pid = 0;
int main()
{

    putStr("kernel\n");
    init_all();
    asm volatile("sti");

    process_execute(u_prog_a, "user_a");
    process_execute(u_prog_b, "user_b");

    // console_put_str("main thread: 0x");
    // console_put_int(sys_get_pid());
    // console_putChar('\n');

    thread_start(thread1, "thread1 ", "thread1 ", 31);
    thread_start(thread2, "thread2 ", "thread2 ", 31);
    while (1)
        ;

    return 0;
}
void thread1(void *arg)
{
    void *addr1 = sys_malloc(256);
    void *addr2 = sys_malloc(255);
    void *addr3 = sys_malloc(254);
    console_put_str("thread 1 addr : 0x");
    console_put_int((uint32_t)addr1);
    console_put_str(",");
    console_put_int((uint32_t)addr2);
    console_put_str(",");
    console_put_int((uint32_t)addr3);
    console_put_str("\n");
    for (int i = 0; i < 100000; ++i)
    {
    }
    sys_free(addr1);
    sys_free(addr2);
    sys_free(addr3);
    while (1)
        ;
    // char *args = arg;
    // void *addr1;
    // void *addr2;
    // void *addr3;
    // void *addr4;
    // void *addr5;
    // void *addr6;
    // void *addr7;

    // console_put_str(" thread1 start:");
    // int max = 0;
    // while (max < 100)
    // {
    //     putInt(max);
    //     putStr("\n");
    //     int size = 128;
    //     addr1 = sys_malloc(size);
    //     size *= 2;
    //     addr2 = sys_malloc(size);
    //     size *= 2;
    //     addr3 = sys_malloc(size);
    //     sys_free(addr1);
    //     addr4 = sys_malloc(size);
    //     size *= 2;
    //     size *= 2;
    //     size *= 2;
    //     size *= 2;
    //     size *= 2;
    //     size *= 2;
    //     size *= 2;
    //     addr5 = sys_malloc(size);
    //     addr6 = sys_malloc(size);
    //     sys_free(addr5);
    //     size *= 2;
    //     addr7 = sys_malloc(size); // 问题在此，打个标记
    //     //struct arena *mem_arena = block_to_arena((struct mem_block *)addr6);
    //     // putStr("mem arena: ");
    //     // putInt(mem_arena->cnt);
    //     // putStr(" ");
    //     // putInt(addr5);
    //     // putStr(" ");
    //     // putInt(addr7);
    //     // putStr(" ");
    //     // putInt(addr6);
    //     sys_free(addr6);
    //     //while(1);
    //     sys_free(addr7);
    //     sys_free(addr2);
    //     sys_free(addr3);
    //     sys_free(addr4);
    //     ++max;
    // }
    // console_put_str(" thread1 end:");

    // while (1)
    //     ;
}
void thread2(void *arg)
{
    void *addr1 = sys_malloc(256);
    void *addr2 = sys_malloc(255);
    void *addr3 = sys_malloc(254);
    console_put_str("thread 2 addr : 0x");
    console_put_int((uint32_t)addr1);
    console_put_str(",");
    console_put_int((uint32_t)addr2);
    console_put_str(",");
    console_put_int((uint32_t)addr3);
    console_put_str("\n");
    for (int i = 0; i < 1000; ++i)
    {
    }
    sys_free(addr1);
    sys_free(addr2);
    sys_free(addr3);
    while (1)
        ;
    // char *args = arg;
    // void *addr1;
    // void *addr2;
    // void *addr3;
    // void *addr4;
    // void *addr5;
    // void *addr6;
    // void *addr7;

    // console_put_str(" thread2 start:");
    // int max = 0;
    // while (max < 100)
    // {
    //     int size = 128;
    //     addr1 = sys_malloc(size);
    //     size *= 2;
    //     addr2 = sys_malloc(size);
    //     size *= 2;
    //     addr3 = sys_malloc(size);
    //     sys_free(addr1);
    //     addr4 = sys_malloc(size);
    //     size *= 2;
    //     size *= 2;
    //     size *= 2;
    //     size *= 2;
    //     size *= 2;
    //     size *= 2;
    //     size *= 2;
    //     addr5 = sys_malloc(size);
    //     addr6 = sys_malloc(size);
    //     sys_free(addr5);
    //     size *= 2;
    //     addr7 = sys_malloc(size);
    //     sys_free(addr6);
    //     sys_free(addr7);
    //     sys_free(addr2);
    //     sys_free(addr3);
    //     sys_free(addr4);
    //     ++max;
    // }
    // console_put_str(" thread2 end:");

    // while (1)
    //     ;
}
void u_prog_a()
{
    void *addr1 = malloc(256);
    void *addr2 = malloc(255);
    void *addr3 = malloc(254);
    printf("userprog 1 addr : 0x%x,0x%x,0x%x\n",
           (int)addr1, (int)addr2, (int)addr3);
    for (int i = 0; i < 1000; ++i)
    {
    }
    free(addr1);
    free(addr2);
    free(addr3);
    while (1)
        ;
}
void u_prog_b()
{
    void *addr1 = malloc(256);
    void *addr2 = malloc(255);
    void *addr3 = malloc(254);
    printf("userprog 2 addr : 0x%x,0x%x,0x%x\n",
           (int)addr1, (int)addr2, (int)addr3);
    for (int i = 0; i < 1000; ++i)
    {
    }
    free(addr1);
    free(addr2);
    free(addr3);
    while (1)
        ;
}

// void thread1(void *arg);
// void thread2(void *arg);
// void u_prog_a();
// void u_prog_b();
// int prog_a_pid = 0, prog_b_pid = 0;
// int main()
// {

//     putStr("kernel\n");
//     init_all();
//     thread_start(thread1, "thread1 ", "thread1 ", 8);
//     thread_start(thread2, "thread2 ", "thread2 ", 8);
//     process_execute(u_prog_a, "user_a");
//     process_execute(u_prog_b, "user_b");

//     asm volatile("sti");

//     while (1)
//         ;

//     return 0;
// }
// void thread1(void *arg)
// {
//     char *args = arg;
//     while (1)
//     {
//         console_put_str("v_a: 0x");
//         console_put_int(prog_a_pid);
//     }
// }
// void thread2(void *arg)
// {
//     char *args = arg;
//     while (1)
//     {
//         console_put_str("v_b: 0x");
//         console_put_int(prog_b_pid);
//     }
// }
// void u_prog_a()
// {
//     while (1)
//     {
//         ++prog_a_pid;
//     }
// }
// void u_prog_b()
// {

//     while (1)
//     {
//         ++prog_b_pid;
//     }
// }