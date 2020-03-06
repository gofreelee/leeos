#include "../lib/stdint.h"
#include "../lib/kernel/print.h"
#include "interrupt.h"
#include "global.h"
#include "io.h"
#include "../device/timer.h"
#define INT_DESC_NUM 0x21 //目前的中断描述符表的大小
#define PIC_M_EVEN 0x20
#define PIC_M_ODD 0x21
#define PIC_S_EVEN 0xa0
#define PIC_S_ODD 0xa1

struct interrupt_gate_desc
{
    //该结构体是中断门描述符
    uint16_t code_offset_low_word;  //在目标代码段的偏移量的低16位
    uint16_t selector;              //中断处理程序目标代码段的段选择子
    uint8_t n_count;                // 用来占位的,
    uint8_t attribute;              // 属性
    uint16_t code_offset_high_word; //在目标代码段的偏移量的高16位
};

//中断描述符表
static struct interrupt_gate_desc idt[INT_DESC_NUM];

//在 kernel.asm 中的中断处理函数的入口数组.
extern intr_handler interrupt_table[INT_DESC_NUM];

//异常处理函数指针数组
intr_handler exception_funcptr_table[INT_DESC_NUM];

static char *exception_names[INT_DESC_NUM]; //名字数组

//设置中断描述符属性的函数
static void set_inerrupt_gate_desc(struct interrupt_gate_desc *intr_ptr, uint8_t attr, intr_handler func)
{
    intr_ptr->n_count = 0; //
    intr_ptr->attribute = attr;
    intr_ptr->selector = CODE_SELECTOR; //
    intr_ptr->code_offset_low_word = (uint32_t)func & (0x0000ffff);
    intr_ptr->code_offset_high_word = ((uint32_t)func & (0xffff0000)) >> 16;
}

static void general_exception_handler(uint8_t vec_number)
{
    if (vec_number == 0x27 || vec_number == 0x2f)
    {
        //对于 会产生伪中断和 保留项,直接返回
        return;
    }
    putStr("int vector :0x");
    putInt(vec_number);
    putChar('\n');
}

// 异常处理的一些的初始化
static void exception_init()
{
    for (int i = 0; i < INT_DESC_NUM; ++i)
    {
        exception_funcptr_table[i] = general_exception_handler;
        exception_names[i] = "No Name";
    }
}

// 中断描述符表的初始化
static void idt_desc_init()
{
    for (int i = 0; i < INT_DESC_NUM; ++i)
    {
        set_inerrupt_gate_desc(&idt[i], IDT_DESC_ATTR_DPL0, interrupt_table[i]);
    }
    putStr("idt init done!\n");
}

//  8259中断控制器的初始化
static void pic_init()
{
    // 主片 ICW1 - ICW4 初始化
    out_byte(PIC_M_EVEN, 0x11);
    out_byte(PIC_M_ODD, 0x20);
    out_byte(PIC_M_ODD, 0x04);
    out_byte(PIC_M_ODD, 0x01);

    //从片 ICW1 - ICW4 初始化
    out_byte(PIC_S_EVEN, 0x11);
    out_byte(PIC_S_ODD, 0x28);
    out_byte(PIC_S_ODD, 0x02);
    out_byte(PIC_S_ODD, 0x01);

    out_byte(PIC_M_ODD, 0xfe);
    out_byte(PIC_S_ODD, 0xff);
    putStr("pic init done \n");
}

void idt_init()
{
    // 中断有关的初始化操作

    // 1. 先完成中断描述符表的初始化
    idt_desc_init();

    // 2. 异常处理的相关的初始化
    exception_init();
    // 3. 完成8259中断控制器的初始化
    pic_init();

    // 加载Idt
    uint64_t idt_data_48 = (sizeof(idt) - 1) | ((uint64_t)(uint32_t)idt << 16);
    asm volatile("lidt %0"
                 :
                 : "m"(idt_data_48));
    putStr("idt init done \n");
}