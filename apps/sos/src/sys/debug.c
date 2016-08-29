#include <stdarg.h>
#include <stdio.h>
#include <sel4/sel4.h>

#include "internal/sys/debug.h"

#define BUFFER_SIZE 256

static void _debugPrint(const char* msg) {
    for (; *msg; ++msg)
        seL4_DebugPutChar(*msg);
}

void _kprintf2(const char* colour, const char* fmt, ...) {
    _debugPrint(colour);

    va_list args;
    char buf[BUFFER_SIZE];
    va_start(args, fmt);
    vsnprintf(buf, BUFFER_SIZE, fmt, args);
    va_end(args);

    _debugPrint(buf);
    _debugPrint("\033[0;0m");
}
