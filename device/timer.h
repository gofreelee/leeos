#ifndef TIMER_H_
#define TIMER_H_
#include "../lib/kernel/print.h"

#define IRQ0_FREQUENCY 100
#define INPUT_FREQUENCY 1193180
#define COUNTER_RO_VALUE INPUT_FREQUENCY / IRQ0_FREQUENCY
#define COUNTER_R0_PORT 0x40
#define COUNTER_RO_NO 0
#define COUNTER_MODE 2
#define READ_WRITE_LATCH 3
#define PIT_CONTROL_PORT 0x43

static void frequency_set(uint8_t counter_port,
                          uint8_t counter_no, uint8_t rwl,
                          uint8_t counter_mode, uint8_t counter_value);

void timer_init();

#endif