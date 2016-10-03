#include <chrono>
#include <assert.h>

extern "C" {
    #include "clock/clock.h"
}

#include "internal/timer/timer.h"

namespace {
    bool is_timer_started;
}

int start_timer(seL4_CPtr interrupt_ep) {
    if (is_timer_started)
        assert(stop_timer() == CLOCK_R_OK);

    try {
        timer::init(Capability<seL4_AsyncEndpointObject, seL4_EndpointBits>(0, interrupt_ep));
        is_timer_started = true;
        return CLOCK_R_OK;
    } catch (...) {
        return CLOCK_R_FAIL;
    }
}

int stop_timer() {
    if (!is_timer_started)
        return CLOCK_R_UINT;

    timer::deinit();
    is_timer_started = false;
    return CLOCK_R_OK;
}

timestamp_t time_stamp() {
    if (!is_timer_started)
        return CLOCK_R_UINT;

    return std::chrono::microseconds(timer::getTimestamp().time_since_epoch()).count();
}

static uint32_t _register_timer(uint64_t delay, timer_callback_t callback, void *data, bool isRecurring) {
    if (!is_timer_started)
        return CLOCK_R_UINT;

    try {
        return timer::setTimer(
            std::chrono::microseconds(delay),
            [callback, data](timer::TimerId id) {callback(id, data);},
            isRecurring
        );
    } catch (...) {
        return 0;
    }
}
uint32_t register_timer(uint64_t delay, timer_callback_t callback, void *data) {
    return _register_timer(delay, callback, data, false);
}
uint32_t register_recurring_timer(uint64_t delay, timer_callback_t callback, void *data) {
    return _register_timer(delay, callback, data, true);
}

int remove_timer(uint32_t id) {
    if (!is_timer_started)
        return CLOCK_R_UINT;

    try {
        timer::clearTimer(id);
        return CLOCK_R_OK;
    } catch (...) {
        return CLOCK_R_FAIL;
    }
}

int timer_interrupt() {
    if (!is_timer_started)
        return CLOCK_R_UINT;

    timer::handleIrq();
    return CLOCK_R_OK;
}
