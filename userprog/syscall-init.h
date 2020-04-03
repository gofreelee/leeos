#ifndef SYSCALL_INIT_H
#define SYSCALL_INIT_H
#include "../stdint.h"
#include "../kernel/memory.h"
uint32_t sys_get_pid();
void syscall_table_init();
uint32_t sys_write(const char *);
void *sys_malloc(uint32_t size);
void sys_free(void *ptr);
#endif