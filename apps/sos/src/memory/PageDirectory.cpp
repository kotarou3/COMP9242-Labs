#include <stdexcept>
#include <string>
#include <system_error>
#include <assert.h>

extern "C" {
    #include <cspace/cspace.h>
    #include <sel4/sel4.h>

    #include "internal/sys/debug.h"
    #include "internal/ut_manager/ut.h"
}

#include "internal/memory/FrameTable.h"
#include "internal/memory/Mappings.h"
#include "internal/memory/PageDirectory.h"
#include "internal/memory/Swap.h"

namespace memory {

///////////////////
// PageDirectory //
///////////////////

PageDirectory::PageDirectory(process::Process& process):
    _process(process)
{}

PageDirectory::PageDirectory(process::Process& process, seL4_ARM_PageDirectory cap):
    _process(process),
    // Externally managed cap, so set it's memory to 0
    _cap(0, cap)
{}

PageDirectory::~PageDirectory() {
    // Clear the page tables first, since they are using the page directory
    _tables.clear();

    // Leak the cap if it's externally managed
    if (_cap.getMemory() == 0)
        _cap.release();
}

size_t PageDirectory::countPages() const noexcept {
    size_t pages = 0;
    for (const auto& table : _tables)
        pages += table.second.countPages();
    return pages;
}

void PageDirectory::reservePages(vaddr_t from, vaddr_t to) {
    from = pageTableAlign(from);
    for (vaddr_t address = from; address < to; address += PAGE_TABLE_SIZE) {
        auto table = _tables.find(_toIndex(address));
        if (table == _tables.end()) {
            table = _tables.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(_toIndex(address)),
                std::forward_as_tuple(*this, _toIndex(address))
            ).first;
        }

        table->second.reservePages();
    }
}

async::future<const MappedPage&> PageDirectory::makeResident(vaddr_t address, const Mapping& map) {
    auto table = _tables.find(_toIndex(address));
    if (table == _tables.end())
        return allocateAndMap(address, map);

    MappedPage* page = table->second.lookup(address, true);
    if (!page)
        return allocateAndMap(address, map);

    if (page->getAttributes() != map.attributes)
        throw std::system_error(ENOSYS, std::system_category(), "Changing page attributes not implemented");

    switch (page->getPage().getStatus()) {
        case memory::Page::Status::INVALID:
        case memory::Page::Status::UNMAPPED:
            assert(false);

        case memory::Page::Status::LOCKED:
        case memory::Page::Status::REFERENCED:
            break;

        case memory::Page::Status::UNREFERENCED:
            page->enableReference(*this);
            break;

        case memory::Page::Status::SWAPPED:
            return page->swapIn().then([=](async::future<void> result) -> const MappedPage& {
                result.get();
                page->enableReference(*this);
                return *page;
            });
    }

    return async::make_ready_future<const MappedPage&>(static_cast<const MappedPage&>(*page));
}

async::future<const MappedPage&> PageDirectory::allocateAndMap(vaddr_t address, Mapping map) {
    return FrameTable::alloc().then([=](auto page) {
        return this->map(page.get(), address, map.attributes);
    }).unwrap().then([=](auto result) {
        const MappedPage& _result = result.get();
        if (map.file) {
            vaddr_t readTo = address;
            size_t readSize = PAGE_SIZE;
            if (readTo == map.start) {
                readTo += map.memoryOffset;
                readSize -= map.memoryOffset;
            }

            // Account for the case when the file size is smaller than the memory size
            readSize = std::min(
                readSize,
                static_cast<size_t>(std::max(
                    static_cast<ssize_t>(map.start + map.memoryOffset + map.fileSize - readTo),
                    0
                ))
            );

            assert(readTo - address < PAGE_SIZE);
            assert(readSize <= PAGE_SIZE);

            if (readSize > 0) {
                off64_t fileStart = readTo - (map.start + map.memoryOffset) + map.fileOffset;
                std::vector<fs::IoVector> iovs = {{
                    .buffer = UserMemory(_process.shared_from_this(), readTo),
                    .length = readSize
                }};
                return map.file->read(iovs, fileStart, true).then([=](auto read) -> const MappedPage& {
                    if (static_cast<size_t>(read.get()) != readSize)
                        throw std::system_error(ENOSYS, std::system_category(), "Unable to load ELF file page from NFS");

                    // this and map must still exist if the read succeeded
                    const MappedPage* result = this->lookup(address);
                    assert(seL4_ARM_Page_Unify_Instruction(
                        result->getPage().getCap(),
                        0, PAGE_SIZE
                    ) == seL4_NoError);

                    return *result;
                });
            }
        }

        return async::make_ready_future<const MappedPage&>(_result);
    });
}

async::future<const MappedPage&> PageDirectory::map(Page page, vaddr_t address, Attributes attributes) {
    auto table = _tables.find(_toIndex(address));
    if (table == _tables.end()) {
        table = _tables.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(_toIndex(address)),
            std::forward_as_tuple(*this, _toIndex(address))
        ).first;
    }

    return table->second.map(std::move(page), address, attributes);
}

void PageDirectory::unmap(vaddr_t address) noexcept {
    auto table = _tables.find(_toIndex(address));
    if (table == _tables.end())
        return;

    table->second.unmap(address);
}

void PageDirectory::clear() noexcept {
    _tables.clear();
}

const MappedPage* PageDirectory::lookup(vaddr_t address, bool noThrow) const {
    auto table = _tables.find(_toIndex(address));
    if (table == _tables.end()) {
        if (noThrow)
            return nullptr;
        throw std::invalid_argument("Address is not mapped");
    }

    return table->second.lookup(address, noThrow);
}

///////////////
// PageTable //
///////////////

PageTable::PageTable(PageDirectory& parent, vaddr_t baseAddress):
    _parent(parent),
    _baseAddress(baseAddress),
    _cap(0, 0) // Defer mapping in the table, since it can block
{}

PageTable::~PageTable() {
    // Clear the pages first, since they're using the page table
    _pages.clear();

    // If we haven't been moved away, unmap the page table
    if (_cap)
        assert(seL4_ARM_PageTable_Unmap(_cap.get()) == seL4_NoError);
}

size_t PageTable::countPages() const noexcept {
    return _pages.size();
}

void PageTable::reservePages() {
    if (!_cap) {
        decltype(_cap) cap;
        int err = seL4_ARM_PageTable_Map(cap.get(), _parent.getCap(), _baseAddress, seL4_ARM_Default_VMAttributes);
        if (err != seL4_NoError)
            throw std::system_error(ENOMEM, std::system_category(), "Failed to map in seL4 page table: " + std::to_string(err));
        _cap = std::move(cap);
    }

    _pages.reserve(PAGE_TABLE_SIZE / PAGE_SIZE);
}

async::future<const MappedPage&> PageTable::map(Page page, vaddr_t address, Attributes attributes) {
    _checkAddress(address);

    if (!_cap) try {
        decltype(_cap) cap;
        int err = seL4_ARM_PageTable_Map(cap.get(), _parent.getCap(), _baseAddress, seL4_ARM_Default_VMAttributes);
        if (err != seL4_NoError)
            throw std::system_error(ENOMEM, std::system_category(), "Failed to map in seL4 page table: " + std::to_string(err));
        _cap = std::move(cap);
    } catch (const std::system_error& e) {
        if (e.code().category() != std::system_category() || e.code().value() != ENOMEM)
            throw;

        auto pagePtr = std::make_shared<Page>(std::move(page));
        return FrameTable::swapOldFrame().then([=](async::future<void> result) {
            result.get();
            return this->map(std::move(*pagePtr), address, attributes);
        });
    }

    if (_pages.count(_toIndex(address)) == 0) {
        auto result = _pages.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(_toIndex(address)),
            std::forward_as_tuple(
                std::move(page),
                _parent,
                address,
                attributes
            )
        );

        return async::make_ready_future<const MappedPage&>(static_cast<const MappedPage&>(result.first->second));
    } else {
        throw std::invalid_argument("Address is already mapped");
    }
}

void PageTable::unmap(vaddr_t address) noexcept {
    address = memory::pageAlign(address);
    _checkAddress(address);
    _pages.erase(_toIndex(address));
}

MappedPage* PageTable::lookup(vaddr_t address, bool noThrow) {
    return const_cast<MappedPage*>(static_cast<const PageTable*>(this)->lookup(address, noThrow));
}

const MappedPage* PageTable::lookup(vaddr_t address, bool noThrow) const {
    _checkAddress(address);

    auto page = _pages.find(_toIndex(address));
    if (page == _pages.end()) {
        if (noThrow)
            return nullptr;
        throw std::invalid_argument("Address is not mapped");
    } else {
        return &page->second;
    }
}

void PageTable::_checkAddress(vaddr_t address) const {
    if (memory::pageAlign(address) != address)
        throw std::invalid_argument("Address is not aligned");
    if ((address & _baseAddress) != _baseAddress)
        throw std::invalid_argument("Address does not belong to this page table");
}

////////////////
// MappedPage //
////////////////

MappedPage::MappedPage(Page page, PageDirectory& directory, vaddr_t address, Attributes attributes):
    _page(std::move(page)),
    _address(address),
    _attributes(attributes)
{
    assert(_page._status == Page::Status::UNMAPPED);
    _page._status = Page::Status::UNREFERENCED;

    enableReference(directory);
}

void MappedPage::enableReference(PageDirectory& directory) {
    assert(_page._status == Page::Status::UNREFERENCED);

    int err = seL4_ARM_Page_Map(
        _page.getCap(), directory.getCap(), _address,
        seL4Rights(), seL4Attributes()
    );
    if (err != seL4_NoError)
        throw std::system_error(ENOMEM, std::system_category(), "Failed to map in page: " + std::to_string(err));

    _page._status = _attributes.locked ? Page::Status::LOCKED : Page::Status::REFERENCED;
    if (_page._resident.frame)
        _page._resident.frame->updateStatus();
}

async::future<void> MappedPage::swapIn() {
    assert(_page._status == Page::Status::SWAPPED);
    return Swap::get().swapIn(_page);
}

seL4_CapRights MappedPage::seL4Rights() const {
    int rights = 0;
    if (_attributes.read)
        rights |= seL4_CanRead;
    if (_attributes.write)
        rights |= seL4_CanRead | seL4_CanWrite; // ARM requires read permissions to write
    if (_attributes.execute)
        rights |= seL4_CanRead; // XXX: No execute right on our version of seL4

    return static_cast<seL4_CapRights>(rights);
}

seL4_ARM_VMAttributes MappedPage::seL4Attributes() const {
    int vmAttributes = seL4_ARM_Default_VMAttributes;
    if (!_attributes.execute)
        vmAttributes |= seL4_ARM_ExecuteNever;
    if (_attributes.notCacheable)
        vmAttributes &= ~seL4_ARM_PageCacheable;

    return static_cast<seL4_ARM_VMAttributes>(vmAttributes);
}

MappedPage::~MappedPage() {
    // If we haven't been moved away, unmap the page
    if (_page) {
        switch (_page._status) {
            case memory::Page::Status::INVALID:
            case memory::Page::Status::UNMAPPED:
                assert(false);

            case Page::Status::LOCKED:
            case Page::Status::REFERENCED:
                assert(seL4_ARM_Page_Unmap(_page.getCap()) == seL4_NoError);
                // Fallthrough

            case Page::Status::UNREFERENCED:
                _page._status = Page::Status::UNMAPPED;
                if (_page._resident.frame)
                    _page._resident.frame->updateStatus();
                break;

            case Page::Status::SWAPPED:
                Swap::get().erase(_page);
                break;
        }
    }
}

}
