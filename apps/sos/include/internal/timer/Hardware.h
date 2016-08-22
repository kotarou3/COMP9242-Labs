#pragma once

#include <memory>

extern "C" {
    #include <sel4/types.h>
}

#include "internal/timer/timer.h"

namespace timer {

class Hardware {
    public:
        Hardware(seL4_CPtr irqEndpoint);
        ~Hardware();

        Hardware(const Hardware&) = delete;
        Hardware& operator=(const Hardware&) = delete;

        Timestamp getTimestamp() const noexcept;

        // Requests when the next IRQ will be sent to the IRQ endpoint and
        // overwrites any previous setting. The next IRQ will also clear this
        // automatically. Note that an IRQ can be sent sooner than what is
        // specified here (e.g., timer overflow), hence the request
        void requestNextIrqTime(Timestamp timestamp) noexcept;

        // Gets when the next IRQ will actually occur
        Timestamp getNextIrqTime() const noexcept;

        // Call this when a clock interrupt is received
        void handleIrq() noexcept;

    private:
        struct Registers;
        std::unique_ptr<volatile Registers, void (&)(volatile Registers*)> _registers;

        seL4_IRQHandler _irqCap;
        Timestamp _counterZeroTimestamp;
};

}
