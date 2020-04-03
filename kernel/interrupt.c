#include "../lib/stdint.h"
#include "../lib/kernel/print.h"
#include "interrupt.h"
#include "global.h"
#include "io.h"
#include "../device/timer.h"

#define INT_DESC_NUM 0x81 //目前的中断描述符表的大小
#define PIC_M_EVEN 0x20
#define PIC_M_ODD 0x21
#define PIC_S_EVEN 0xa0
#define PIC_S_ODD 0xa1
#define EFLAG_IF_ON 0x00000200 // eflag 寄存器对应的标志位应该被设置为1
#define GET_EFLAGS(EFLAG) asm volatile("pushfl; pop %0" \
                                       : "=g"(EFLAG))
extern uint32_t system_call_handler();
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
extern void set_cursor(uint32_t cursor);
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

    //光标移动到左上角
    set_cursor(0);
    int cursor = 0;
    while (cursor < 320)
    {
        putChar(' ');
        ++cursor;
    }
    set_cursor(0);
    putStr("!!!!   exception message begin  !!!!\n");
    set_cursor(88);
    putStr(exception_names[vec_number]);
    if (vec_number == 14)
    {
        uint32_t page_fault_vaddr = 0;
        asm volatile("movl %%cr2, %0"
                     : "=r"(page_fault_vaddr));
        putStr("\npage fault addr is ");
        putInt(page_fault_vaddr);
    }
    putStr("\n!!!!   exception message end    !!!!\n");
    while (1)
        ;
}

// 异常处理的一些的初始化
static void exception_init()
{
    for (int i = 0; i < INT_DESC_NUM; ++i)
    {
        exception_funcptr_table[i] = general_exception_handler;
        exception_names[i] = "No Name";
    }
    exception_names[0] = "#DF Divide Error";
    exception_names[1] = "#DB Debug Exception";
    exception_names[2] = "NMI Interrupt";
    exception_names[3] = "#BP Breakpoint Exception";
    exception_names[4] = "#OF Overflow Exception";
    exception_names[5] = "#BR BOUND Range Exceeded Exception";
    exception_names[6] = "#UD Invalid Opcode Exception";
    exception_names[7] = "#NM Device Not Available Exception";
    exception_names[8] = "#DF Double Fault Exception";
    exception_names[9] = "Coprocessor Segment Overrun";
    exception_names[10] = "#TS Invalid TSS Excrption";
    exception_names[11] = "#NP Segment Not Present";
    exception_names[12] = "#SS Stack Fault Exception";
    exception_names[13] = "#GP General Protection Exception";
    exception_names[14] = "#PF Page-Fault Exception";
    // intr_name[15] = "";	保留项
    exception_names[16] = "#MF x87 FPU Floating-Point Error";
    exception_names[17] = "#AC Alignment Check Exception";
    exception_names[18] = "#MC Machine-Check Exception";
    exception_names[19] = "#XF SIMD Floating-Point Exception";
    exception_names[0x20] = "#clock";
}

// 中断描述符表的初始化
static void idt_desc_init()
{
    int system_call_index = INT_DESC_NUM - 1;
    for (int i = 0; i < INT_DESC_NUM; ++i)
    {
        set_inerrupt_gate_desc(&idt[i], IDT_DESC_ATTR_DPL0, interrupt_table[i]);
    }
    set_inerrupt_gate_desc(&idt[system_call_index], IDT_DESC_ATTR_DPL3, system_call_handler);
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

    out_byte(PIC_M_ODD, 0xfc); //fd是只开键盘中断
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

enum intr_status get_intr_status()
{
    uint32_t eflag;
    GET_EFLAGS(eflag);
    return eflag & EFLAG_IF_ON ? INTR_ON : INTR_OFF;
}

static void set_intr_status(enum intr_status status)
{
    if (status)
        intr_open();
    else
        intr_close();
}

void intr_open()
{
    enum intr_status curr_status = get_intr_status();
    if (curr_status)
        return;
    else
    {
        asm volatile("sti");
    }
}

void intr_close()
{
    if (get_intr_status())
    {
        asm volatile("cli"
                     :
                     :
                     : "memory");
    }
}

void register_handler(uint8_t vec_num, intr_handler func)
{
    /*注册处理中断函数 */
    exception_funcptr_table[vec_num] = func;
}

/*************************************************** */
/**
 * 开中断并返回开中断前的状态
 */
enum intr_status intr_enable()
{
    enum intr_status old_status;
    if (INTR_ON == intr_get_status())
    {
        old_status = INTR_ON;
        return old_status;
    }
    else
    {
        old_status = INTR_OFF;
        asm volatile("sti"); // 开中断
        return old_status;
    }
}

/**
 * 关中断并返回关中断前的状态
 */
enum intr_status intr_disable()
{
    enum intr_status old_status;
    if (INTR_ON == intr_get_status())
    {
        old_status = INTR_ON;
        asm volatile("cli"
                     :
                     :
                     : "memory"); // 关中断
        return old_status;
    }
    else
    {
        old_status = INTR_OFF;
        return old_status;
    }
}

/**
 * 将中断状态设置为status
 */
enum intr_status intr_set_status(enum intr_status status)
{
    return status & INTR_ON ? intr_enable() : intr_disable();
}

/**
 * 拿到中断状态
 */
enum intr_status intr_get_status()
{
    uint32_t eflags = 0;
    GET_EFLAGS(eflags);
    return (EFLAG_IF_ON & eflags) ? INTR_ON : INTR_OFF;
}