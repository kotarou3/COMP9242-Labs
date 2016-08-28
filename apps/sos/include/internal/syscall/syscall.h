#pragma once

#define syscall(...) _syscall(__VA_ARGS__)
#include <boost/thread/future.hpp>
#undef syscall

#include "internal/process/Thread.h"

extern "C" {
    #include <sel4/types.h>
}

namespace syscall {

boost::future<int> handle(process::Thread& thread, long number, size_t argc, seL4_Word* argv) noexcept;

namespace {
    inline boost::future<int> _returnNow(int result) {
        boost::promise<int> promise;
        promise.set_value(result);
        return promise.get_future();
    }
}

}
