#include <stdexcept>
#include <system_error>

extern "C" {
    #include <cspace/cspace.h>
    #include "internal/ut_manager/ut.h"
}

#include "internal/fs/File.h"
#include "internal/memory/FrameTable.h"
#include "internal/memory/Swap.h"
#include "internal/memory/UserMemory.h"
#include "internal/process/Thread.h"

namespace memory {

boost::inline_executor Swap::asyncExecutor;

Swap::Swap():
    _bufferMapping(process::getSosProcess().maps.insert(
        0, PARALLEL_SWAPS,
        Attributes{
            .read = true,
            .write = true,
            .execute = false,
            .notCacheable = false,
            .locked = true
        },
        Mapping::Flags{.shared = false}
    )),
    _bufferIoVectors({
        fs::IoVector{
            .buffer = UserMemory(process::getSosProcess(), _bufferMapping.getAddress()),
            .length = 0
        }
    })
{}

void Swap::addBackingStore(std::shared_ptr<fs::File> store, size_t size) {
    if (pageOffset(size) != 0)
        throw std::invalid_argument("Swap file size is not page aligned");
    if (_store)
        throw std::system_error(ENOSYS, std::system_category(), "Multiple swap files not implemented");

    _store = std::move(store);
    _usedBitset.resize(pageCount(size) / PARALLEL_SWAPS);
}

boost::future<void> Swap::swapOut(FrameTable::Frame** frames, size_t frameCount) {
    // TODO: Better mutual exclusion
    static bool isSwapping;
    if (isSwapping)
        throw std::bad_alloc();

    if (frameCount > PARALLEL_SWAPS)
        frameCount = PARALLEL_SWAPS;

    SwapId id = _allocate();

    for (size_t f = 0; f < frameCount; ++f) {
        assert(frames[f]->pages);
        assert(!frames[f]->isLocked);
        assert(!frames[f]->isReferenced);

        try {
            process::getSosProcess().pageDirectory.map(
                frames[f].pages->copy(),
                _bufferMapping.getAddress() + f * PAGE_SIZE,
                Attributes{.read = true}
            );
        } catch (...) {
            frameCount = f;
            break;
        }
    }

    if (frameCount == 0)
        throw std::bad_alloc();

    try {
        isSwapping = true;

        _bufferIoVectors[0].length = frameCount * PAGE_SIZE;
        return store->write(_bufferIoVectors, id * PARALLEL_SWAPS * PAGE_SIZE).then(
            fs::asyncExecutor, [=] (auto written) {
                isSwapping = false;
                for (size_t f = 0; f < frameCount; ++f)
                    process::getSosProcess().pageDirectory.unmap(_bufferMapping.getAddress() + f * PAGE_SIZE);

                try {
                    if (written.get() != frameCount * PAGE_SIZE)
                        throw std::bad_alloc();
                } catch (...) {
                    _free(id);
                    throw;
                }

                for (size_t f = 0; f < frameCount; ++f) {
                    for (Page* page = frames[f]->pages; page != nullptr; page = page->next) {
                        assert(page->_status == Page::Status::UNREFERENCED);
                        assert(page->_resident.frame = frames[f]);

                        assert(cspace_delete_cap(cur_cspace, page->_resident.cap) == CSPACE_NOERROR);

                        page->_status = Page::Status::SWAPPED;
                        page->_swapId = id * PARALLEL_SWAPS + f;
                    }

                    ut_free(frames[f]->getAddress(), seL4_PageBits);
                    frames[f]->pages = nullptr;
                }
            }
        );
    } catch (...) {
        for (size_t f = 0; f < frameCount; ++f)
            process::getSosProcess().pageDirectory.unmap(_bufferMapping.getAddress() + f * PAGE_SIZE);
        _free(id);

        isSwapping = false;
        throw;
    }
}

SwapId Swap::_allocate() {
    size_t i = (_lastUsed + 1) % _usedBitset.size();
    for (; i != _lastUsed; i = (i + 1) % _usedBitset.size()) {
        if (_usedBitset[i])
            continue;

        _usedBitset[i] = true;
        _lastUsed = i;
        return i;
    }

    throw std::bad_alloc();
}

void Swap::_free(SwapId id) noexcept {
    _usedBitset[id] = false;
}

}
