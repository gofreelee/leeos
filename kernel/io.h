#ifndef IO_H_
#define IO_H_
#include "../lib/stdint.h"
#include "../lib/kernel/print.h"
static inline void out_byte(uint16_t port, uint8_t data)
{
    //该函数是用来向指定端口写入数据的
    asm volatile("outb %b0, %w1" ::"a"(data), "Nd"(port)); // Nd?
}
static inline void out_multibyte(uint16_t port, const void *addr, uint32_t count)
{
    //写入多字节数据, outsw 指令   ds: esi
    asm volatile("cld; rep outsw"
                 : "+S"(addr), "+c"(count)
                 : "d"(port));
}

static inline uint8_t in_byte(uint16_t port)
{
    uint8_t res;
    asm volatile("in %w1 , %b0"
                 : "=a"(res)
                 : "Nd"(port));
    return res;
}

static inline void in_multibyte(uint16_t port, const void *addr, uint32_t count)
{
    asm volatile("cld;rep insw"
                 : "+D"(addr), "+c"(count)
                 : "Nd"(port)
                 : "memory");
}

#endif