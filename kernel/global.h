#ifndef GLOBAL_H_
#define GLOBAL_H_
#include "../lib/stdint.h"

#define RPL0 0
#define RPL1 1
#define RPL2 2
#define RPL3 3

#define TI_GDT 0
#define TI_LDT 1

// 之前定义的一些段选择子
#define CODE_SELECTOR ((0x01 << 3) + (TI_GDT << 2) + RPL0)
#define DATA_SELECTOR ((0x02 << 3) + (TI_GDT << 2) + RPL0)
#define STACK_SELECTOR DATA_SELECTOR
#define GS_SELECTOR   ((0x03 << 3) + (TI_GDT << 2) + RPL0)

// IDT 描述符属性
#define IDT_DESC_P      1
#define IDT_DESC_DPL0   0
#define IDT_DESC_DPL1   1
#define IDT_DESC_DPL2   2
#define IDT_DESC_DPL3   3


#define IDT_DESC_32_TYPE 0xe
#define IDT_DESC_16_TYPE 0x6 

#define IDT_DESC_ATTR_DPL0 \
((IDT_DESC_P << 7) + (IDT_DESC_DPL0 << 5) + IDT_DESC_32_TYPE)
#define IDT_DESC_ATTR_DPL3 \
((IDT_DESC_P << 7) + (IDT_DESC_DPL3 << 5) + IDT_DESC_32_TYPE)
#endif