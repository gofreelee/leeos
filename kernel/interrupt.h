#ifndef INTERRUPT_H_
#define INTERRUPT_H_

//中断处理函数指针
typedef void *intr_handler;
void idt_init();

//枚举类型 , 1 表示中断已经打开, 0 表示中断关上了
enum intr_status
{
    INTR_OFF,
    INTR_ON
};

enum intr_status get_intr_status();
static void set_intr_status(enum intr_status new_status);

//中断打开
void intr_open();

//中断关闭
void intr_close();
void register_handler(uint8_t vec_num, intr_handler func);

enum intr_status intr_enable();
enum intr_status intr_disable();
enum intr_status intr_set_status(enum intr_status status);
enum intr_status intr_get_status();
#endif