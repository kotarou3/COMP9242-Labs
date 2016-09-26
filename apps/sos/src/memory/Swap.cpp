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

Swap::Swap():
    _bufferMapping(process::getSosProcess().maps.insert(
        0, 1,
        Attributes{
            .read = true,
            .write = true,
            .execute = false,
            .locked = true
        },
        Mapping::Flags{.shared = false}
    )),
    _bufferIoVectors({
        fs::IoVector{
            .buffer = UserMemory(process::getSosProcess(), _bufferMapping.getAddress()),
            .length = PAGE_SIZE
        }
    })
{}

void Swap::addBackingStore(std::shared_ptr<fs::File> store, size_t size) {
    if (pageOffset(size) != 0)
        throw std::invalid_argument("Swap file size is not page aligned");
    if (_store)
        throw std::system_error(ENOSYS, std::system_category(), "Multiple swap files not implemented");

    _store = std::move(store);
    _usedBitset.resize(numPages(size));
}

async::future<void> Swap::swapOut(FrameTable::Frame& frame) {
    if (_isSwapping || !_store)
        throw std::bad_alloc();

    assert(frame._pages);
    assert(!frame._isLocked);
    assert(!frame._isReferenced);

    SwapId id = _allocate();

    Attributes attributes = {0};
    attributes.read = true;
    attributes.locked = true;
    process::getSosProcess().pageDirectory.map(
        frame._pages->copy(),
        _bufferMapping.getAddress(),
        attributes
    );

    try {
        _isSwapping = true;

        return _store->write(_bufferIoVectors, id * PAGE_SIZE).then([=, &frame](auto written) {
            _isSwapping = false;

            process::getSosProcess().pageDirectory.unmap(_bufferMapping.getAddress());

            try {
                if (static_cast<size_t>(written.get()) != PAGE_SIZE)
                    throw std::bad_alloc();
            } catch (...) {
                this->_free(id);
                throw;
            }

            for (Page* page = frame._pages; page != nullptr; page = page->_next) {
                assert(page->_status == Page::Status::UNREFERENCED);
                assert(page->_resident.frame == &frame);

                assert(cspace_delete_cap(cur_cspace, page->_resident.cap) == CSPACE_NOERROR);

                page->_status = Page::Status::SWAPPED;
                page->_swapId = id;
            }

            ut_free(frame.getAddress(), seL4_PageBits);
            frame._pages = nullptr;
        });
    } catch (...) {
        _isSwapping = false;

        process::getSosProcess().pageDirectory.unmap(_bufferMapping.getAddress());
        _free(id);

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
