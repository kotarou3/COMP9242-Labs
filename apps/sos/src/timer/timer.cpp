#include <exception>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>

extern "C" {
    #include <sel4/sel4.h>

    #define verbose 2
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
            return this->time > q.time;
        }
    };

    TimerId nextFree;
    std::unordered_map<TimerId, Timer> timers;
    std::set<TimerId> toRemove;

    std::priority_queue<Interrupt> interrupts;
}

void init(seL4_CPtr irqEndpoint) {
    hardware = std::make_unique<timer::Hardware>(irqEndpoint);
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
        throw std::runtime_error("Timer delay cannot be non-positive");

    while (timers.count(nextFree) || nextFree == 0)
        ++nextFree;

    timers[nextFree] = {
        .delay = delay,
        .callback = callback,
        .recurring = recurring
    };

    auto interruptTime = getTimestamp() + delay;
    interrupts.push(Interrupt{.time = interruptTime, .id = nextFree});

    // this may be earlier than the next interrupt
    if (hardware->getNextIrqTime() > interruptTime)
        hardware->requestNextIrqTime(interruptTime);

    return nextFree++;
}

void clearTimer(TimerId id) {
    // mark for removal. Actual removal only occurs upon removing the interrupt
    if (timers.count(id) == 0)
        throw std::runtime_error("Timer id #" + std::to_string(id) + " does not exist");

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
                    kprintf(0, "Skipped %zu callbacks for id #%zu\n", skippedCallbacks, id);

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
