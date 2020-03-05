;中断处理程序
[bits 32]
extern putStr ; 引入外部函数
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
    push interrupt
    call putStr
    add esp, 4

    mov al, 0x20
    out 0x20,al
    out 0xa0,al

    add esp, 4
    iret
section .data
    dd  interrupt%1entry

%endmacro

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
