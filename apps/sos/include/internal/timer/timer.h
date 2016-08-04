#pragma once

#include <chrono>
#include <functional>
#include <ratio>

extern "C" {
    #include <sel4/sel4.h>
}

namespace timer {

struct clock {
    // TrivialClock interface

    using rep = uint64_t;
    using period = std::micro;
    using duration = std::chrono::duration<rep, period>;
    using time_point = std::chrono::time_point<clock>;
    static constexpr bool is_steady = false; // Would be true if not for deinit()

    static inline time_point now() noexcept;
};

using Timestamp = clock::time_point;
using Duration = clock::duration;
using TimerId = size_t;

/**
 * Initialise/deinitialise driver.
 * @param irqEndpoint A (possibly badged) async endpoint that the driver should
 *                        use for delivering interrupts to.
 */
void init(seL4_CPtr irqEndpoint);
void deinit() noexcept;

/**
 * Returns present time since the last init().
 */
Timestamp getTimestamp() noexcept;
inline Timestamp clock::now() noexcept {
    return getTimestamp();
}

/**
 * Register a callback to be called after a given delay or interval
 * @param delay Delay or interval time.
 * @param callback Function to be called.
 * @param isRecurring Whether to call the callback recurringly.
 * @return Non-zero TimerId
 */
TimerId setTimer(Duration delay, const std::function<void (TimerId)>& callback, bool isRecurring = false);
static inline TimerId setTimer(Duration delay, const std::function<void ()>& callback, bool isRecurring = false) {
    return setTimer(delay, [callback](TimerId) {callback();}, isRecurring);
}

/**
 * Remove a previously registered callback by its ID
 * @param id Unique ID returned by setTimer()
 */
void clearTimer(TimerId id);

/**
 * Handle an interrupt message sent to irqEndpoint from init()
 */
void handleIrq() noexcept;

}
