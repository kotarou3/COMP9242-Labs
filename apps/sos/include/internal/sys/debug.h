#pragma once

void _kprintf2(const char* colour, const char* fmt, ...);

#define _kprintf(v, colour, ...)        \
    do {                                \
        if ((v) < verbose)              \
            _kprintf2(colour, __VA_ARGS__); \
    } while (0)

#define kprintf(v, ...) _kprintf(v, "\033[22;33m", __VA_ARGS__)

#define WARN(...) _kprintf(-1, "\033[1;31mWARNING: ", __VA_ARGS__)
