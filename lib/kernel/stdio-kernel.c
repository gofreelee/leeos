#include "stdio-kernel.h"
#include "../../device/console.h"
#define va_start(ap, v) ap = (va_list)&v;
#define va_arg(ap, t) *((t *)(ap += 4))
#define va_end(ap) ap = 0
typedef char *va_list;

uint32_t printk(const char *format, ...)
{
    va_list ap;
    //uint32_t len;
    va_start(ap, format);
    char buf[1024] = {0};
    vsprintf(buf, format, ap);
    va_end(ap);
    console_put_str(buf);
    return 0;
}
