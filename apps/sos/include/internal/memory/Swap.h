#pragma once

#include <vector>

#define syscall(...) _syscall(__VA_ARGS__)
// Includes missing from inline_executor.hpp
#include <boost/thread/thread.hpp>
#include <boost/thread/concurrent_queues/sync_queue.hpp>

#include <boost/thread/executors/inline_executor.hpp>
#include <boost/thread/future.hpp>
#undef syscall

#include "internal/memory/Mappings.h"

namespace fs {
    class File;
    struct IoVector;
}

namespace memory {

constexpr const size_t PARALLEL_SWAPS = 16;

namespace FrameTable {
    struct Frame;
}

using SwapId = size_t;
class Swap {
    public:
        void addBackingStore(std::shared_ptr<fs::File> store, size_t size);

        boost::future<void> swapOut(FrameTable::Frame** frames, size_t frameCount);

        static Swap& get() noexcept {
            static Swap swap;
            return swap;
        }

        static boost::inline_executor asyncExecutor;

    private:
        Swap();

        SwapId _allocate();
        void _free(SwapId id) noexcept;

        std::shared_ptr<fs::File> _store;

        std::vector<bool> _usedBitset;
        size_t _lastUsed;

        ScopedMapping _bufferMapping;
        std::vector<fs::IoVector> _bufferIoVectors;
};

}
