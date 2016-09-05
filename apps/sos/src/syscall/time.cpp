#include <chrono>
#include <memory>
#include <stdexcept>

#include <assert.h>
#include <errno.h>
#include <time.h>

#include "internal/memory/UserMemory.h"
#include "internal/syscall/time.h"
#include "internal/timer/timer.h"

namespace syscall {

boost::future<int> clock_gettime(process::Process& process, clockid_t clk_id, memory::vaddr_t tp) {
    if (clk_id != CLOCK_REALTIME)
        throw std::invalid_argument("Unknown clock ID");

    using namespace std::chrono;

    auto timestamp = timer::getTimestamp().time_since_epoch();
    auto secs = duration_cast<seconds>(timestamp);

    memory::UserMemory(process, tp).set(timespec{
        .tv_sec = static_cast<time_t>(secs.count()),
        .tv_nsec = static_cast<long>((nanoseconds(timestamp) - nanoseconds(secs)).count())
    });

    return boost::make_ready_future(0);
}

boost::future<int> nanosleep(process::Process& process, memory::vaddr_t req, memory::vaddr_t /*rem*/) {
    timespec _req = memory::UserMemory(process, req).get<timespec>();
    if (_req.tv_sec < 0 || !(0 <= _req.tv_nsec && _req.tv_nsec <= 999999999))
        throw std::invalid_argument("Invalid sleep time");

    auto promise = std::make_shared<boost::promise<int>>();
    using namespace std::chrono;
    timer::setTimer(
        duration_cast<microseconds>(seconds(_req.tv_sec) + nanoseconds(_req.tv_nsec)),
        [promise] {promise->set_value(0);}
    );

    return promise->get_future();
}

}

extern "C" int sys_nanosleep() {
    assert(!"SOS cannot sleep");
    __builtin_unreachable();
}

FORWARD_SYSCALL(clock_gettime, 2);
