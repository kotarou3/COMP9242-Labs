#pragma once

#include <random>

// System random device that should be using a CSPRNG, not MT

class RandomDevice {
    public:
        using RNG = std::mt19937;
        using result_type = RNG::result_type;

        result_type operator()() noexcept {return _rng();};
        double entropy() const noexcept {return 0;};

        static constexpr result_type min() noexcept {return RNG::min();};
        static constexpr result_type max() noexcept {return RNG::max();};

        static RandomDevice& getSingleton() noexcept;

    private:
        RandomDevice() = default;

        RandomDevice(const RandomDevice&) = delete;
        RandomDevice& operator=(const RandomDevice&) = delete;

        RNG _rng;
};
