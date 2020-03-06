;中断处理程序
[bits 32]
extern putStr ; 引入外部函数
extern exception_funcptr_table ; 引入异常处理函数指针数组

%define ERROR_CODE nop
%define ZERO    push 0

section .data

interrupt db "interrupt happens!",0x0a,0 ; 字符串数据

global interrupt_table ; 导出外部数据

interrupt_table:

%macro interrupt_deal 2
section .text

interrupt%1entry:

    %2
    ;保存一些寄存器
    push ds
    push es
    push fs
    push gs
    pushad

   
    mov al, 0x20 ; 发送EOI 给主片和从片
    out 0x20,al
    out 0xa0,al

    push %1

    call [exception_funcptr_table + %1 * 4] ; 调用对应的中断处理函数
    add esp, 4
    jmp exit_intr

section .data
    dd  interrupt%1entry

%endmacro

section .text
global exit_intr  ; 导出中断退出

exit_intr:

popad
pop gs
pop fs
pop es
pop ds

add esp, 4
iretd




interrupt_deal 0x00, ZERO
interrupt_deal 0x01, ZERO
interrupt_deal 0x02, ZERO
interrupt_deal 0x03, ZERO
interrupt_deal 0x04, ZERO
interrupt_deal 0x05, ZERO
interrupt_deal 0x06, ZERO
interrupt_deal 0x07, ZERO
interrupt_deal 0x08, ZERO
interrupt_deal 0x09, ZERO
interrupt_deal 0x0a, ZERO
interrupt_deal 0x0b, ZERO
interrupt_deal 0x0c, ZERO
interrupt_deal 0x0d, ZERO
interrupt_deal 0x0e, ZERO
interrupt_deal 0x0f, ZERO
interrupt_deal 0x10, ZERO
interrupt_deal 0x11, ZERO
interrupt_deal 0x12, ZERO
interrupt_deal 0x13, ZERO
interrupt_deal 0x14, ZERO
interrupt_deal 0x15, ZERO
interrupt_deal 0x16, ZERO
interrupt_deal 0x17, ZERO
interrupt_deal 0x18, ZERO
interrupt_deal 0x19, ZERO
interrupt_deal 0x1a, ZERO
interrupt_deal 0x1b, ZERO
interrupt_deal 0x1c, ZERO
interrupt_deal 0x1d, ZERO
interrupt_deal 0x1e, ERROR_CODE
interrupt_deal 0x1f, ZERO
interrupt_deal 0x20, ZERO
