#include "debug.h"
#include "../lib/kernel/print.h"
#include "interrupt.h"

void exception_report(const char *file, int line,
                      const char *func, const char *error_source)
{
    // 报告错误要把中断关上.
    intr_close();
    putStr("\n\n\n!!!!! error !!!!!!\n");

    putStr("filename: ");
    putStr(file);
    putChar('\n');

    putStr("error line: ");
    putStr("0x");
    putInt(line);
    putChar('\n');

    putStr("error function: ");
    putStr(func);
    putChar('\n');

    putStr("error_source: ");
    putStr(error_source);
    putChar('\n');
    while (1)
        ;
}