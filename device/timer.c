#include "../thread/thread.h"
#include "../kernel/io.h"
#include "timer.h"
#include "debug.h"
#include "../kernel/interrupt.h"
#define DIV_ROUND_UP(X, STEP) (X + STEP - 1) / STEP

// 时钟控制寄存器
uint32_t ticks; // 时钟总滴答数
static void frequency_set(uint8_t counter_port,
                          uint8_t counter_no, uint8_t rwl,
                          uint8_t counter_mode, uint8_t counter_value)
{
    //counter_port 是要写入的端口 counter_no 是控制字里的选择计数器
    // rwl 是读写方式,供CPU读, counter_mode 是X10,
    //先写入 时钟控制寄存器
    out_byte(PIT_CONTROL_PORT, (uint8_t)(counter_no << 6 | rwl << 4 | counter_mode << 1));
    out_byte(counter_port, (uint8_t)(counter_value));
    out_byte(counter_port, (uint8_t)((counter_value) >> 8));
}

/*时钟中断处理函数 */
static void intr_timer_handler()
{
    /*获取当前线程的pcb */
    struct pcb_struct *curr_thread = running_thread();
    // putStr(" enter timer handler :");
    // putInt((uint32_t)curr_thread);
    // putChar('\n');
    /*判断线程的pcb指针是否溢出 */
    ASSERT(curr_thread->stack_magic == 0x00007c00);

    ++(curr_thread->elapsed_ticks);
    ++ticks;
    if (curr_thread->ticks == 0)
    {
        schedule();
    }
    else
    {
        --(curr_thread->ticks);
    }
}

void timer_init()
{
    frequency_set(COUNTER_R0_PORT, COUNTER_RO_NO, READ_WRITE_LATCH,
                  COUNTER_MODE, COUNTER_RO_VALUE);
    ticks = 0;
    register_handler(0x20, intr_timer_handler);
    putStr("timer init done\n");
}

static void ticks_to_sleep(uint32_t sleep_ticks)
{
    uint32_t start_ticks = ticks;
    while ((ticks - start_ticks) > sleep_ticks)
    {
        thread_yield();
    }
}

void mtime_sleep(uint32_t m_seconds)
{
    uint32_t frequency = 1000 / IRQ0_FREQUENCY; // 一次中断多少毫秒
    ASSERT(frequency >= 0)
    uint32_t ticks = DIV_ROUND_UP(m_seconds, frequency);
    ticks_to_sleep(ticks);
}