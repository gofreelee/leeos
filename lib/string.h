#ifndef STRING_H_
#define STRING_H_
#include "stdint.h"
void memset(void *dst, uint8_t value, uint32_t num);
void memcpy(void *dst, void *src, uint32_t size);
int memcmp(const void *a, const void *b, uint32_t size);
char *strcpy(char *dst, const char *src);
uint32_t strlen(const char *str);
int strcmp(const char *a, const char *b);
char *strchr(const char *str, const uint8_t ch); // 从前往后第一次出现的地址
char *strrchr(const char *str, const uint8_t ch);
char *strcat(char *dst, const char *src);
uint32_t strchrs(const char *dst, char ch);
#endif