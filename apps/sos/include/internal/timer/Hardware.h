#pragma once

extern "C" {
    #include <sel4/types.h>
}

#include "internal/memory/DeviceMemory.h"
#include "internal/timer/timer.h"

namespace timer {

class Hardware {
    public:
        Hardware(Capability<seL4_AsyncEndpointObject, seL4_EndpointBits> irqEndpoint);
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
        memory::DeviceMemory<Registers> _registers;

        Capability<seL4_AsyncEndpointObject, seL4_EndpointBits> _irqEndpoint;
        seL4_IRQHandler _irqCap;

        Timestamp _counterZeroTimestamp;
};

}
