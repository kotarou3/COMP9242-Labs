#pragma once

// From linux
#define LOGLEVEL_EMERG      0   // system is unusable
#define LOGLEVEL_ALERT      1   // action must be taken immediately
#define LOGLEVEL_CRIT       2   // critical conditions
#define LOGLEVEL_ERR        3   // error conditions
#define LOGLEVEL_WARNING    4   // warning conditions
#define LOGLEVEL_NOTICE     5   // normal but significant condition
#define LOGLEVEL_INFO       6   // informational
#define LOGLEVEL_DEBUG      7   // debug-level messages

#define KPRINTF_VERBOSITY LOGLEVEL_ERR

void _kprintf2(const char* colour, const char* fmt, ...);

#define _kprintf(v, colour, ...)        \
    do {                                \
        if ((v) <= KPRINTF_VERBOSITY)              \
            _kprintf2(colour, __VA_ARGS__); \
    } while (0)

#define kprintf(v, ...) _kprintf(v, "\033[22;33m", __VA_ARGS__)

#define WARN(...) _kprintf(LOGLEVEL_WARNING, "\033[1;31mWARNING: ", __VA_ARGS__)
