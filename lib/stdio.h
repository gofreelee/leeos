#ifndef STDIO_H_
#define STDIO_H_
#include "stdint.h"

typedef char *va_list;
void itoa(uint32_t number, char **str, uint32_t base);
uint32_t vsprintf(char *str, const char *format, va_list ap);
uint32_t printf(const char *format, ...);
uint32_t sprintf(char *buf, const char *format, ...);

#endif