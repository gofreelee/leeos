#include "string.h"
#include "kernel/debug.h"
void memset(void *dst, uint8_t value, uint32_t num)
{
    ASSERT(dst != 0)
    int index = 0;
    for (; index < num; ++index)
        *((uint8_t *)dst + index) = value;
}

void memcpy(void *dst, void *src, uint32_t size)
{
    ASSERT(dst != 0 && src != 0)
    int index = 0;
    for (; index < size; ++index)
    {
        *((uint8_t *)dst + index) = *((uint8_t *)src + index);
    }
}

int memcmp(const void *a_, const void *b_, uint32_t size)
{
    ASSERT(a_ != 0 && b_ != 0)
    uint8_t *a = a_, *b = b_;
    while (size-- > 0)
    {
        if (*a != *b)
            return *a > *b ? 1 : -1;
        ++a;
        ++b;
    }
    return 0;
}

char *strcpy(char *dst_, const char *src_)
{
    ASSERT(dst_ != 0 && src_ != 0)
    char *res = dst_;
    uint8_t *dst = dst_, *src = src_;
    while (*src != '\0')
    {
        *dst++ = *src++;
    }
    *dst = '\0';
    return res;
}

uint32_t strlen(const char *str)
{
    ASSERT(str != 0)
    uint32_t len = 0;
    while (*str++ != '\0')
        ++len;
    return len;
}

int strcmp(const char *a, const char *b)
{
    ASSERT(a != 0 && b != 0)
    while (*a != '\0' && *a == *b)
    {
        ++a;
        ++b;
    }
    return *a < *b ? -1 : *a > *b;
}

char *strchr(const char *str, const uint8_t ch)
{
    ASSERT(str != 0)
    while (*str != '\0')
    {
        if (*str == ch)
            return (char *)str;
        ++str;
    }
    return 0;
}

char *strrchr(const char *str, const uint8_t ch)
{
    ASSERT(str != 0)
    const char *res;
    while (*str != '0')
    {
        if (*str == ch)
            res = str;
        ++str;
    }
    return (char *)res;
}

char *strcat(char *dst, const char *src)
{
    ASSERT(dst != 0 && src != 0)
    char *dst_ = dst;
    while (*dst != '\0')
        ++dst;
    while (*src != '\0')
    {
        *dst = *src;
        ++dst;
        ++src;
    }
    *dst = '\0';
    return dst_;
}

uint32_t strchrs(const char *dst, char ch)
{
    uint32_t counter = 0;
    while(*dst != '\0')
    {
        if(*dst == ch)
            ++counter;
        ++dst;
    }
    return counter;
}