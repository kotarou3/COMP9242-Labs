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
#include "internal/process/Process.h"

namespace memory {

Swap::Swap():
    _swapOutBufferMapping(process::getSosProcess()->maps.insert(
        0, 1,
        Attributes{
            .read = true,
            .write = false,
            .execute = false,
            .locked = true
        },
        Mapping::Flags{.shared = false}
    )),
    _swapOutBufferIoVectors({
        fs::IoVector{
            .buffer = UserMemory(process::getSosProcess(), _swapOutBufferMapping.getAddress()),
            .length = PAGE_SIZE
        }
    }),
    _swapInBufferMapping(process::getSosProcess()->maps.insert(
        0, 1,
        Attributes{
            .read = false,
            .write = true,
            .execute = false,
            .locked = true
        },
        Mapping::Flags{.shared = false}
    )),
    _swapInBufferIoVectors({
        fs::IoVector{
            .buffer = UserMemory(process::getSosProcess(), _swapInBufferMapping.getAddress()),
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
    if (!_store)
        throw std::bad_alloc();

    auto promise = std::make_shared<async::promise<void>>();
    _pendingSwapOuts.push([=, &frame]() noexcept {
        assert(frame._pages);
        assert(!frame._isLocked);
        assert(!frame._isReferenced);

        try {
            SwapId id = _allocate();

            try {
                Attributes attributes = {0};
                attributes.read = true;
                attributes.locked = true;
                auto mapResult = process::getSosProcess()->pageDirectory.map(
                    frame._pages->copy(),
                    _swapOutBufferMapping.getAddress(),
                    attributes
                );
                assert(mapResult.is_ready());
                mapResult.get();

                try {
                    _store->write(_swapOutBufferIoVectors, id * PAGE_SIZE).then([=, &frame](auto written) {
                        process::getSosProcess()->pageDirectory.unmap(_swapOutBufferMapping.getAddress());

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
                    }).then([=](async::future<void> result) noexcept {
                        try {
                            result.get();
                            promise->set_value();
                        } catch (...) {
                            promise->set_exception(std::current_exception());
                        }

                        _pendingSwapOuts.pop();
                        if (!_pendingSwapOuts.empty())
                            _pendingSwapOuts.front()();
                    });
                } catch (...) {
                    process::getSosProcess()->pageDirectory.unmap(_swapOutBufferMapping.getAddress());
                    throw;
                }
            } catch (...) {
                _free(id);
                throw;
            }
        } catch (...) {
            promise->set_exception(std::current_exception());

            _pendingSwapOuts.pop();
            if (!_pendingSwapOuts.empty())
                _pendingSwapOuts.front()();
        }
    });

    if (_pendingSwapOuts.size() == 1)
        _pendingSwapOuts.front()();

    return promise->get_future();
}

async::future<void> Swap::swapIn(const Page& page) {
    if (!_store)
        throw std::bad_alloc();

    assert(page._status == Page::Status::SWAPPED);
    assert(_usedBitset[page._swapId]);

    auto targetPage = std::make_shared<Page>(page.copy());
    auto promise = std::make_shared<async::promise<void>>();

    _pendingSwapIns.push([this, targetPage, promise]() noexcept {
        FrameTable::alloc().then([=](auto bufferPage) {
            Page _bufferPage = std::move(bufferPage.get());
            seL4_Word bufferPageCap = _bufferPage.getCap();
            FrameTable::Frame& bufferFrame = *_bufferPage._resident.frame;

            try {
                Attributes attributes = {0};
                attributes.read = true;
                attributes.write = true;
                attributes.locked = true;
                auto mapResult = process::getSosProcess()->pageDirectory.map(
                    std::move(_bufferPage),
                    _swapInBufferMapping.getAddress(),
                    attributes
                );
                assert(mapResult.is_ready());
                mapResult.get();

                try {
                    _store->read(_swapInBufferIoVectors, targetPage->_swapId * PAGE_SIZE)
                        .then([=, &bufferFrame](auto read) {
                            try {
                                if (static_cast<size_t>(read.get()) != PAGE_SIZE)
                                    throw std::bad_alloc();
                            } catch (...) {
                                process::getSosProcess()->pageDirectory.unmap(_swapInBufferMapping.getAddress());
                                throw;
                            }

                            Page* head = targetPage.get();
                            while (head->_prev)
                                head = head->_prev;

                            SwapId id = targetPage->_swapId;

                            for (Page* page = head; page != nullptr; page = page->_next) {
                                assert(page->_status == Page::Status::SWAPPED);
                                assert(page->_swapId == id);

                                page->_resident.cap = cspace_copy_cap(cur_cspace, cur_cspace, bufferPageCap, seL4_AllRights);
                                if (page->_resident.cap == CSPACE_NULL) {
                                    // Rollback
                                    page->_swapId = id;
                                    for (page = page->_prev; page != nullptr; page = page->_prev) {
                                        assert(cspace_delete_cap(cur_cspace, page->_resident.cap) == CSPACE_NOERROR);
                                        page->_status = Page::Status::SWAPPED;
                                        page->_swapId = id;
                                    }

                                    process::getSosProcess()->pageDirectory.unmap(_swapInBufferMapping.getAddress());
                                    throw std::system_error(ENOMEM, std::system_category(), "Failed to copy page cap");
                                }
                                assert(page->_resident.cap != 0);

                                page->_status = Page::Status::UNREFERENCED;
                                page->_resident.frame = &bufferFrame;
                            }

                            assert(!bufferFrame._pages->_next);
                            bufferFrame._pages->_next = head;
                            head->_prev = bufferFrame._pages;

                            assert(seL4_ARM_Page_Unify_Instruction(
                                bufferPageCap,
                                0, PAGE_SIZE
                            ) == seL4_NoError);

                            process::getSosProcess()->pageDirectory.unmap(_swapInBufferMapping.getAddress());
                            this->_free(id);

                            targetPage->_status = Page::Status::UNMAPPED;
                        }).then([=](async::future<void> result) noexcept {
                            try {
                                result.get();
                                promise->set_value();
                            } catch (...) {
                                promise->set_exception(std::current_exception());
                            }

                            _pendingSwapIns.pop();
                            if (!_pendingSwapIns.empty())
                                _pendingSwapIns.front()();
                        });
                } catch (...) {
                    process::getSosProcess()->pageDirectory.unmap(_swapInBufferMapping.getAddress());
                    throw;
                }
            } catch (...) {
                promise->set_exception(std::current_exception());

                _pendingSwapIns.pop();
                if (!_pendingSwapIns.empty())
                    _pendingSwapIns.front()();
            }
        });
    });

    if (_pendingSwapIns.size() == 1)
        _pendingSwapIns.front()();

    return promise->get_future();
}

void Swap::copy(const Page& from, Page& to) noexcept {
    assert(from._status == Page::Status::SWAPPED);
    assert(to._status == Page::Status::INVALID);
    assert(_usedBitset[from._swapId]);

    to._status = Page::Status::SWAPPED;
    to._swapId = from._swapId;

    to._prev = const_cast<Page*>(&from);
    to._next = from._next;
    to._prev->_next = &to;
    if (to._next)
        to._next->_prev = &to;
}

void Swap::erase(Page& page) noexcept {
    assert(page._status == Page::Status::SWAPPED);
    assert(_usedBitset[page._swapId]);

    if (page._prev)
        page._prev->_next = page._next;
    if (page._next)
        page._next->_prev = page._prev;

    if (!page._prev && !page._next) {
        // Last page
        _free(page._swapId);
    }

    page._status = Page::Status::INVALID;
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
