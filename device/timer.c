
#include "../kernel/io.h"
#include "timer.h"
// 时钟控制寄存器
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

static void timer_init()
{
    frequency_set(COUNTER_R0_PORT, COUNTER_RO_NO, READ_WRITE_LATCH,
                  COUNTER_MODE, COUNTER_RO_VALUE);
    putStr("timer init done\n");
}