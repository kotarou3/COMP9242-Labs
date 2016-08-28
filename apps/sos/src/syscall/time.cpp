#include <chrono>
#include <exception>
#include <memory>

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>

#include "internal/memory/UserMemory.h"
#include "internal/process/Thread.h"
#include "internal/syscall/time.h"
#include "internal/timer/timer.h"

namespace syscall {

boost::future<int> clock_gettime(process::Process& process, clockid_t clk_id, memory::vaddr_t tp) noexcept {
    if (clk_id != CLOCK_REALTIME)
        return _returnNow(-EINVAL);

    auto timestamp = timer::getTimestamp().time_since_epoch();

    try {
        using namespace std::chrono;
        auto secs = duration_cast<seconds>(timestamp);

        memory::UserMemory(process, tp).set(timespec{
            .tv_sec = static_cast<time_t>(secs.count()),
            .tv_nsec = static_cast<long>((nanoseconds(timestamp) - nanoseconds(secs)).count())
        });

        return _returnNow(0);
    } catch (...) {
        return _returnNow(-EFAULT);
    }
}

boost::future<int> nanosleep(process::Process& process, memory::vaddr_t req, memory::vaddr_t /*rem*/) noexcept {
    try {
        timespec _req = memory::UserMemory(process, req).get<timespec>();
        if (_req.tv_sec < 0 || !(0 <= _req.tv_nsec && _req.tv_nsec <= 999999999))
            return _returnNow(-EINVAL);

        auto promise = std::make_shared<boost::promise<int>>();
        using namespace std::chrono;
        timer::setTimer(
            duration_cast<microseconds>(seconds(_req.tv_sec) + nanoseconds(_req.tv_nsec)),
            [promise] {promise->set_value(0);}
        );

        return promise->get_future();
    } catch (...) {
        return _returnNow(-EFAULT);
    }
}

}

extern "C" {

int sys_clock_gettime(clockid_t clk_id, memory::vaddr_t tp) {
    auto result = syscall::clock_gettime(process::getSosProcess(), clk_id, tp);
    assert(result.is_ready());
    return result.get();
}

int sys_nanosleep() {
    assert(!"SOS cannot sleep");
    __builtin_unreachable();
}

}
