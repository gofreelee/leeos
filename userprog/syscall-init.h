#ifndef SYSCALL_INIT_H
#define SYSCALL_INIT_H
#include "../stdint.h"
uint32_t sys_get_pid();
void syscall_table_init();
uint32_t sys_write(const char *);
#endif