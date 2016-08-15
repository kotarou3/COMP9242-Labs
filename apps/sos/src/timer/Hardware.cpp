#include <algorithm>
#include <exception>
#include <string>
#include <stdint.h>

extern "C" {
    #include <cspace/cspace.h>
    #include <platsupport/clock.h>
    #include <platsupport/plat/gpt_constants.h>

    #include "internal/mapping.h"
}

#include "internal/timer/Hardware.h"

namespace timer {

using std::chrono::microseconds;

struct Hardware::Registers {
    uint32_t control;
    uint32_t prescaler;
    uint32_t status;
    uint32_t interrupt;

    uint32_t outputCompare[3];
    uint32_t inputCapture[2];
    uint32_t counter;
};

Hardware::Hardware(seL4_CPtr irqEndpoint):
    _registers(
        static_cast<volatile Registers*>(map_device(reinterpret_cast<void*>(GPT1_DEVICE_PADDR), sizeof(Registers))),
        reinterpret_cast<void (&)(volatile Registers*)>(unmap_device)
    )
{
    // Set the timer to use the peripheral clock
    _registers->control = 0;
    _registers->control = GPT1_CR_SWR;
    while (_registers->control & GPT1_CR_SWR)
        ;
    _registers->control |= GPT1_CR_CLKSRC_PERIPHERAL;

    // Count up by 1 per microsecond
    clock_sys_t clock;
    assert(!clock_sys_init_default(&clock));
    freq_t freq = clk_get_freq(clk_get_clock(&clock, CLK_IPG));
    if (freq % 1000000 != 0)
        throw std::runtime_error("Non-integer MHz peripheral clock not supported");
    _registers->prescaler = freq / 1000000 - 1;

    // Enable interrupts only on overflow for now
    _registers->control |= GPT1_CR_FRR;
    _registers->interrupt = GPT1_IR_ROVIE;

    // Since one of our functions is a monotonic clock, we need to stay enabled
    // in low power modes
    _registers->control |= GPT1_CR_STOPEN | GPT1_CR_DOZEEN | GPT1_CR_WAITEN;

    // Set the endpoint to send IRQs to
    _irqCap = cspace_irq_control_get_cap(cur_cspace, seL4_CapIRQControl, GPT1_INTERRUPT);
    if (_irqCap == CSPACE_NULL)
        throw std::runtime_error("Failed to get IRQ capability");
    if (int err = seL4_IRQHandler_SetEndpoint(_irqCap, irqEndpoint)) {
        assert(cspace_delete_cap(cur_cspace, _irqCap) == CSPACE_NOERROR);
        throw std::runtime_error("Failed to set IRQ endpoint: " + std::to_string(err));
    }

    // Enable the timer!
    _registers->control |= GPT1_CR_EN;
}

Hardware::~Hardware() {
    _registers->control = 0;
    _registers->control = GPT1_CR_SWR;

    assert(seL4_IRQHandler_Clear(_irqCap) == 0);
    assert(cspace_delete_cap(cur_cspace, _irqCap) == CSPACE_NOERROR);
}

Timestamp Hardware::getTimestamp() const noexcept {
    bool isAlreadyOverflowed = _registers->status & GPT1_SR_ROV;
    Timestamp timestamp = _counterZeroTimestamp + microseconds(_registers->counter);

    if (isAlreadyOverflowed) {
        // Counter has overflowed, but it hasn't been handled yet
        return timestamp + microseconds(0x100000000);
    } else if (_registers->status & GPT1_SR_ROV) {
        // Counter wasn't overflown initially, but did during this function body,
        // so now our timestamp is potentially off by 0x100000000, but we can't
        // be sure about that, so we re-run the function
        return getTimestamp();
    } else {
        // Counter not overflowed
        return timestamp;
    }
}

void Hardware::requestNextIrqTime(Timestamp timestamp) noexcept {
    timestamp = std::max(
        timestamp,
        // Since the IRQ only occurs when the counter *changes* (i.e., cannot be
        // 0 ticks) and the counter might increment while executing this code
        // (cannot be 1 tick), the soonest "safe" interrupt is in 2us since this
        // function is short enough to execute in 1us
        getTimestamp() + microseconds(2)
    );

    uint64_t counterTimestamp = (timestamp - _counterZeroTimestamp).count();
    if (counterTimestamp <= 0xffffffff) {
        // Enable the compare interrupt at the specified time
        _registers->outputCompare[0] = counterTimestamp;
        _registers->interrupt |= GPT1_IR_OF1IE;
    } else {
        // Too far in the future, so we just disable the compare interrupt and
        // let the overflow interrupt be next
        _registers->interrupt &= ~GPT1_IR_OF1IE;
    }
}

Timestamp Hardware::getNextIrqTime() const noexcept {
    if (_registers->interrupt & GPT1_IR_OF1IE)
        return _counterZeroTimestamp + microseconds(_registers->outputCompare[0]);
    else
        return _counterZeroTimestamp + microseconds(0x100000000);
}

void Hardware::handleIrq() noexcept {
    if (_registers->status & GPT1_SR_ROV)
        _counterZeroTimestamp += microseconds(0x100000000);

    // Clear status register
    _registers->status = 0xffffffff;

    assert(seL4_IRQHandler_Ack(_irqCap) == 0);
}

}
