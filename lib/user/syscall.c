#include "syscall.h"

#define _syscall0(NUMBER)           \
    ({                              \
        int retval;                 \
        asm volatile("int $0x80"    \
                     : "=a"(retval) \
                     : "a"(NUMBER)  \
                     : "memory");   \
        retval;                     \
    })

#define _syscall1(NUMBER, ARGV1)               \
    ({                                         \
        int retval;                            \
        asm volatile("int $0x80"               \
                     : "=a"(retval)            \
                     : "a"(NUMBER), "b"(ARGV1) \
                     : "memory");              \
        retval;                                \
    })

#define _syscall2(NUMBER, ARGV1, ARGV2)                    \
    ({                                                     \
        int retval;                                        \
        asm volatile("int $0x80"                           \
                     : "=a"(retval)                        \
                     : "a"(NUMBER), "b"(ARGV1), "c"(ARGV2) \
                     : "memory");                          \
        retval;                                            \
    })

#define _syscall3(NUMBER, ARGV1, ARGV2, ARGV3)                         \
    ({                                                                 \
        int retval;                                                    \
        asm volatile("int $0x80"                                       \
                     : "=a"(retval)                                    \
                     : "a"(NUMBER), "b"(ARGV1), "c"(ARGV2), "d"(ARGV3) \
                     : "memory");                                      \
        retval;                                                        \
    })
uint32_t getpid()
{
    return _syscall0(SYS_GETPID);
}

uint32_t write(const char *str)
{
    return _syscall1(SYS_WRITE, str);
}

void *malloc(uint32_t size)
{
    return _syscall1(SYS_MALLOC, size);
}

void free(void *addr)
{
    return _syscall1(SYS_FREE, addr);
}