#ifndef THREAD_H_
#define THREAD_H_
#include "../lib/stdint.h"

/*声明一个名为　thread_func 的类型，　代指一类函数 */

typedef void thread_func(void *);

/*线程/进程的状态 */
enum task_status
{
    TASK_RUNNING, //运行态
    TASK_READY,   //就绪态
    TASK_BLOCKED, // 阻塞态
    TASK_WAITING, // 等待
    TASK_HANGING, // 挂起态
    TASK_DIED     // 莫得了
};

/*中断发送时，被压入栈的参数 */
struct intr_stack
{
    uint32_t vec_number; // 中断向量号

    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_dummy; //popad　的时候并不会　把esp　返回，因为esp本来是一直在变化
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;

    /*特权级发送变化　才压　ss esp */
    uint32_t error_code;
    uint32_t eflags;
    void (*eip)(void);
    uint32_t cs;
    void *esp;
    uint32_t ss;
};

struct thread_stack
{
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edi;
    uint32_t esi;

    void (*eip)(thread_func *func, void *func_args);

    void *unuse_addr; // 占位的
    thread_func *function;
    void *function_args;
};

struct pcb_struct
{
    uint32_t *self_kernel_stack; //内核栈的指针
    enum task_status status;
    uint8_t priority; // 线程优先级
    char name[16];
    uint32_t stack_magic; // 魔数，用来检测栈边界
};
void thread_start(thread_func func, void *func_args,
                  const char *name, int prio);

#endif