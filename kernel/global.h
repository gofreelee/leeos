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
#define GS_SELECTOR ((0x03 << 3) + (TI_GDT << 2) + RPL0)

// IDT 描述符属性
#define IDT_DESC_P 1
#define IDT_DESC_DPL0 0
#define IDT_DESC_DPL1 1
#define IDT_DESC_DPL2 2
#define IDT_DESC_DPL3 3

#define IDT_DESC_32_TYPE 0xe
#define IDT_DESC_16_TYPE 0x6

#define IDT_DESC_ATTR_DPL0 \
    ((IDT_DESC_P << 7) + (IDT_DESC_DPL0 << 5) + IDT_DESC_32_TYPE)
#define IDT_DESC_ATTR_DPL3 \
    ((IDT_DESC_P << 7) + (IDT_DESC_DPL3 << 5) + IDT_DESC_32_TYPE)

/*----------------------GDT 描述符号属性--------------------------*/
#define DESC_G_4K 1
#define DESC_D_32 1 // 默认的操作数的位数，　1表示为32位
#define DESC_L 0    // 64位代码标记，这里设置为０即可
#define DESC_AVL 0  //cpu 不用此位
#define DESC_P 1    // 段存在位
//下面四个是特权级
#define DESC_DPL_0 0
#define DESC_DPL_1 1
#define DESC_DPL_2 2
#define DESC_DPL_3 3

/*****************************************************
 * 代码段和数据段属于存储段，而tss和各种门描述符是属于系统段
 * s = 1 表示存储段，　s = 0 表示系统段
 * 
 ******************************************************/
#define DESC_S_CODE 1
#define DESC_S_DATA DESC_S_CODE
#define DESC_S_SYS 0
#define DESC_TYPE_CODE 8
// 8:1000  代码段是可执行的　非依从的　不可读的　未访问的

#define DESC_TYPE_DATA 2
// 2:0010  数据段是不可执行的　向上拓展　可写　未访问的

#define DESC_TYPE_TSS 9 // tss对应的系统段

#define SELECTOR_K_CODE ((1 << 3) + (TI_GDT << 2) + RPL0)
#define SELECTOR_K_DATA ((2 << 3) + (TI_GDT << 2) + RPL0)
#define SELECTOR_K_STACK SELECTOR_K_DATA
// 显存
#define SELECTOR_K_GS ((3 << 3) + (TI_GDT << 2) + RPL0)
#define SELECTOR_U_CODE ((5 << 3) + (TI_GDT << 2) + RPL3)
#define SELECTOR_U_DATA ((6 << 3) + (TI_GDT << 2) + RPL3)
#define SELECTOR_U_STACK SELECTOR_U_DATA

#define GDT_ATTR_HIGH ((DESC_G_4K << 7) + (DESC_D_32 << 6) + (DESC_L << 5) + (DESC_AVL << 4))
#define GDT_ATTR_CODE_LOW_DPL_3 \
    ((DESC_P << 7) + (DESC_DPL_3 << 5) + (DESC_S_CODE << 4) + DESC_TYPE_CODE)

#define GDT_ATTR_DATA_LOW_DPL_3 \
    ((DESC_P << 7) + (DESC_DPL_3 << 5) + (DESC_S_DATA << 4) + DESC_TYPE_DATA)

/*----------------------TSS 描述符号属性--------------------------*/
#define TSS_DESC_D 0
#define TSS_ATTR_HIGH \
    ((DESC_G_4K << 7) + (TSS_DESC_D << 6) + (DESC_L << 5) + (DESC_AVL << 4) + 0x00)
#define TSS_ATTR_LOW \
    ((DESC_P << 7) + (DESC_DPL_0 << 5) + (DESC_S_SYS << 4) + DESC_TYPE_TSS)

#define SELECTOR_TSS ((4 << 3) + (TI_GDT << 2) + RPL0)

// gdt描述符
struct gdt_desc
{
    uint16_t limit_low_word; // 段界限最低的两个字节
    uint16_t base_low_word;  // 段基址最低的两个字节
    uint8_t base_mid_byte;   //  段基址第三个字节
    uint8_t attr_low_byte;
    uint8_t limit_high_attr_high;
    uint8_t base_high_byte;
};

/*------------------------------------------------------ */
#define EFLAGS_MBS (1 << 1)
#define EFLAGS_IF_1 (1 << 9) //IF 为　１
#define EFLAGS_IF_0 0
#define EFLAGS_IOPL_3 (3 << 12)
#define EFLAGS_IOPL_0 (0 << 12)


#endif