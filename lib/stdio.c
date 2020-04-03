#include "stdio.h"
#include "stdint.h"
#include "string.h"
#include "user/syscall.h"

#define va_start(ap, v) ap = (va_list)&v;
#define va_arg(ap, t) *((t *)(ap += 4))
#define va_end(ap) ap = 0

void itoa(uint32_t number, char **str, uint32_t base)
{
    uint32_t remainder = number % base;
    uint32_t divRes = number / base;
    if (divRes != 0)
    {
        itoa(divRes, str, base);
    }
    if (remainder < 10)
    {
        *((*str)++) = remainder + '0';
    }
    else
    {
        *((*str)++) = remainder - 10 + 'A';
    }
}

uint32_t vsprintf(char *str, const char *format, va_list ap)
{
    /*把format里的东西拷贝到str 遇到　％ 就格式化*/
    const char *format_index = format;
    char *str_index = str, *str_copy; // str_copy是帮助处理％s
    uint32_t number;
    while (*format_index != '\0')
    {
        if (*format_index != '%')
        {
            *str_index = *format_index;
            ++str_index;
            ++format_index;
            continue;
        }
        ++format_index;
        switch (*format_index)
        {
        case 'x':
            number = va_arg(ap, int);
            itoa(number, &str_index, 16);
            ++format_index;

            break;
        case 's':
            str_copy = va_arg(ap, char *);
            strcpy(str_index, str_copy);
            str_index += strlen(str_copy);
            ++format_index;
            break;
        case 'c':
            *str_index = va_arg(ap, char); // 单独获取一个字符
            ++str_index;
            ++format_index;
            break;

        case 'd':
            number = va_arg(ap, int);
            if (number < 0)
            {
                *str_index = '-';
                ++str_index;
                number = 0 - number;
            }
            itoa(number, &str_index, 10);
            ++format_index;
            break;
        default:
            break;
        }
    }
    return strlen(str);
}

uint32_t printf(const char *format, ...)
{
    va_list ap;
    //uint32_t len;
    va_start(ap, format);
    char buf[1024] = {0};
    vsprintf(buf, format, ap);
    va_end(ap);
    return write(buf);
}

uint32_t sprintf(char *buf, const char *format, ...)
{
    va_list ap;
    uint32_t len;
    va_start(ap, format);
    len = vsprintf(buf, format, ap);
    va_end(ap);
    return len;
}