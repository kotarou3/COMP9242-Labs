#pragma once

#include <vector>

#include "internal/async.h"
#include "internal/fs/File.h"
#include "internal/memory/Mappings.h"

namespace memory {

namespace FrameTable {
    struct Frame;
}

using SwapId = size_t;
class Swap {
    public:
        void addBackingStore(std::shared_ptr<fs::File> store, size_t size);

        async::future<void> swapOut(FrameTable::Frame& frame);
        async::future<void> swapIn(Page& page);

        void copy(const Page& from, Page& to) noexcept;
        void erase(Page& page) noexcept;

        static Swap& get() noexcept {
            static Swap swap;
            return swap;
        }

    private:
        Swap();

        SwapId _allocate();
        void _free(SwapId id) noexcept;

        std::shared_ptr<fs::File> _store;

        std::vector<bool> _usedBitset;
        size_t _lastUsed = -1U;

        ScopedMapping _bufferMapping;
        std::vector<fs::IoVector> _bufferIoVectors;

        // TODO: Better mutual exclusion
        bool _isSwapping = false;
};

}
