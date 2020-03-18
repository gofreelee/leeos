#ifndef CONSOLE_H_
#define CONSOLE_H_
#include "../lib/stdint.h"

void console_init();
void console_acquire();
void console_release();
void console_put_str(char *str);
void console_putChar(uint8_t char_ascii);
void console_put_int(uint32_t num);
#endif