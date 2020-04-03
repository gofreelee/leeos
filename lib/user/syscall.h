#ifndef SYSCALL_H_
#define SYSCALL_H_
#include "../stdint.h"
enum syscall_number
{
    SYS_GETPID,
    SYS_WRITE,
    SYS_MALLOC,
    SYS_FREE
};

uint32_t getpid();
uint32_t write(const char *);
void *malloc(uint32_t size);
void free(void *addr);
#endif