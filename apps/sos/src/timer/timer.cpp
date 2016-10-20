#include <queue>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>

extern "C" {
    #include <sel4/types.h>

    #include "internal/sys/debug.h"
}

#include "internal/timer/Hardware.h"
#include "internal/timer/timer.h"

namespace timer {

namespace {
    std::unique_ptr<timer::Hardware> hardware;

    struct Timer {
        Duration delay;
        std::function<void (TimerId)> callback;
        bool recurring;
    };
    struct Interrupt {
        Timestamp time;
        TimerId id;
        bool operator<(const Interrupt& q) const {
            // reverse it so we get the smaller one off the PQ first
            return time > q.time;
        }
    };

    TimerId nextFree;
    std::unordered_map<TimerId, Timer> timers;
    std::set<TimerId> toRemove;

    std::priority_queue<Interrupt> interrupts;
}

void init(Capability<seL4_AsyncEndpointObject, seL4_EndpointBits> irqEndpoint) {
    hardware = std::make_unique<timer::Hardware>(std::move(irqEndpoint));
    nextFree = 1;
}

void deinit() noexcept {
    interrupts = {};
    toRemove.clear();
    timers.clear();

    hardware.reset();
}

Timestamp getTimestamp() noexcept {
    return hardware->getTimestamp();
}

TimerId setTimer(Duration delay, const std::function<void (TimerId)>& callback, bool recurring) {
    if (delay.count() <= 0 && recurring)
        throw std::invalid_argument("Timer delay cannot be non-positive");

    while (timers.count(nextFree) || nextFree == 0)
        ++nextFree;

    timers[nextFree] = {
        .delay = delay,
        .callback = callback,
        .recurring = recurring
    };

    auto now = getTimestamp();
    auto interruptTime = now + delay;
    // if they ask for an interrupt infeasibly far into the future, tell them it worked but don't add it
    // it requires 584,000 years since system start, so I think we're good
    if (interruptTime >= now) {
        interrupts.push(Interrupt{.time = interruptTime, .id = nextFree});

        // this may be earlier than the next interrupt
        if (hardware->getNextIrqTime() > interruptTime)
            hardware->requestNextIrqTime(interruptTime);
    }

    return nextFree++;
}

void clearTimer(TimerId id) {
    // mark for removal. Actual removal only occurs upon removing the interrupt
    // can't remove from the middle of a priority queue, so this is the next best solution
    if (timers.count(id) == 0)
        throw std::invalid_argument("Timer id #" + std::to_string(id) + " does not exist");

    toRemove.insert(id);
}

void handleIrq() noexcept {
    hardware->handleIrq();
    if (interrupts.empty())
        return;

    auto nextInterrupt = interrupts.top().time;
    while (nextInterrupt <= getTimestamp()) {
        auto id = interrupts.top().id;
        interrupts.pop();

        if (toRemove.count(id)) {
            // remove the timer now that it no longer has a corresponding interrupt
            timers.erase(id);
            toRemove.erase(id);
        } else {
            // run the callback function, and push the next one on
            timers[id].callback(id);
            if (timers[id].recurring) {
                size_t skippedCallbacks = (getTimestamp() - nextInterrupt) / timers[id].delay;
                if (skippedCallbacks > 0)
                    kprintf(LOGLEVEL_NOTICE, "Skipped %zu callbacks for id #%zu\n", skippedCallbacks, id);

                // this could potentially overflow since it's recurring, but to be the interval to go over
                // it has to be long enough to ensure that it overflows, but short enough to ensure that it doesn't
                // overflow in the initial settimer. Worst case is 292,000 years till it breaks...
                interrupts.push(Interrupt{
                    .time = nextInterrupt + (skippedCallbacks + 1) * timers[id].delay,
                    .id = id
                });
            } else {
                timers.erase(id);
            }
        }

        if (interrupts.empty())
            return;
        nextInterrupt = interrupts.top().time;
    }

    hardware->requestNextIrqTime(nextInterrupt);
}

}
