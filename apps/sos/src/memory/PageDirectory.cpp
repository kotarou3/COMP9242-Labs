#include <stdexcept>
#include <string>
#include <system_error>
#include <assert.h>

#include "internal/memory/FrameTable.h"
#include "internal/memory/PageDirectory.h"

extern "C" {
    #include <cspace/cspace.h>
    #include <sel4/sel4.h>

    #include "internal/ut_manager/ut.h"
}

namespace memory {

///////////////////
// PageDirectory //
///////////////////

PageDirectory::PageDirectory(seL4_ARM_PageDirectory cap):
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

const MappedPage& PageDirectory::allocateAndMap(vaddr_t address, Attributes attributes) {
    return map(FrameTable::alloc(), address, attributes);
}

const MappedPage& PageDirectory::map(Page page, vaddr_t address, Attributes attributes) {
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

void PageDirectory::unmap(vaddr_t address) {
    _tables.at(_toIndex(address)).unmap(address);
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
    _baseAddress(baseAddress)
{
    int err = seL4_ARM_PageTable_Map(_cap.get(), _parent.getCap(), baseAddress, seL4_ARM_Default_VMAttributes);
    if (err != seL4_NoError)
        throw std::system_error(ENOMEM, std::system_category(), "Failed to map in seL4 page table: " + std::to_string(err));
}

PageTable::~PageTable() {
    // Clear the pages first, since they're using the page table
    _pages.clear();

    // If we haven't been moved away, unmap the page table
    if (_cap)
        assert(seL4_ARM_PageTable_Unmap(_cap.get()) == seL4_NoError);
}

const MappedPage& PageTable::map(Page page, vaddr_t address, Attributes attributes) {
    _checkAddress(address);

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

        return result.first->second;
    } else {
        throw std::invalid_argument("Address is already mapped");
    }
}

void PageTable::unmap(vaddr_t address) {
    _checkAddress(address);
    _pages.erase(_toIndex(address));
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
    int rights = 0;
    if (attributes.read)
        rights |= seL4_CanRead;
    if (attributes.write)
        rights |= seL4_CanRead | seL4_CanWrite; // ARM requires read permissions to write
    if (attributes.execute)
        rights |= seL4_CanRead; // XXX: No execute right on our version of seL4

    int vmAttributes = seL4_ARM_Default_VMAttributes;
    if (!attributes.execute)
        vmAttributes |= seL4_ARM_ExecuteNever;
    if (attributes.notCacheable)
        vmAttributes &= ~seL4_ARM_PageCacheable;

    int err = seL4_ARM_Page_Map(
        _page.getCap(), directory.getCap(), address,
        static_cast<seL4_CapRights>(rights),
        static_cast<seL4_ARM_VMAttributes>(vmAttributes)
    );
    if (err != seL4_NoError)
        throw std::system_error(ENOMEM, std::system_category(), "Failed to map in page: " + std::to_string(err));
}

MappedPage::~MappedPage() {
    // If we haven't been moved away, unmap the page
    if (_page)
        assert(seL4_ARM_Page_Unmap(_page.getCap()) == seL4_NoError);
}

}
