#include <stdexcept>
#include <string>
#include <assert.h>

#include "internal/memory/FrameTable.h"
#include "internal/memory/PageDirectory.h"

extern "C" {
    #include <cspace/cspace.h>

    #include "internal/ut_manager/ut.h"
}

namespace memory {

PageDirectory sosPageDirectory(seL4_CapInitThreadPD);

///////////////////
// PageDirectory //
///////////////////

PageDirectory::PageDirectory(seL4_ARM_PageDirectory cap):
    _cap(cap)
{}

std::vector<std::reference_wrapper<const MappedPage>> PageDirectory::allocateAndMap(size_t pages, vaddr_t address, Attributes attributes) {
    if (pages == 0)
        throw std::invalid_argument("Must specify at least one page");

    std::vector<std::reference_wrapper<const MappedPage>> result;
    result.reserve(pages);

    for (size_t p = 0; p < pages; ++p) {
        vaddr_t curAddress = address + (p << seL4_PageBits);
        result.push_back(map(FrameTable::alloc(), curAddress, attributes));
    }

    return result;
}

const MappedPage& PageDirectory::map(Page page, vaddr_t address, Attributes attributes) {
    if (_tables.count(_toIndex(address)) == 0) {
        _tables.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(_toIndex(address)),
            std::forward_as_tuple(*this, _toIndex(address))
        );
    }

    return _tables.at(_toIndex(address)).map(std::move(page), address, attributes);
}

void PageDirectory::unmap(vaddr_t address) {
    _tables.at(_toIndex(address)).unmap(address);
}

const MappedPage& PageDirectory::lookup(vaddr_t address) const {
    return _tables.at(_toIndex(address)).lookup(address);
}

///////////////
// PageTable //
///////////////

PageTable::PageTable(PageDirectory& parent, vaddr_t baseAddress):
    _parent(parent),
    _baseAddress(baseAddress)
{
    _memory = ut_alloc(seL4_PageTableBits);
    if (_memory == 0)
        throw std::runtime_error("Failed to allocate seL4 memory");

    int err = cspace_ut_retype_addr(
        _memory, seL4_ARM_PageTableObject, seL4_PageTableBits, cur_cspace, &_cap
    );
    if (err != seL4_NoError) {
        ut_free(_memory, seL4_PageTableBits);
        throw std::runtime_error("Failed to retype seL4 memory to page table: " + std::to_string(err));
    }

    err = seL4_ARM_PageTable_Map(_cap, parent.getCap(), baseAddress, seL4_ARM_Default_VMAttributes);
    if (err != seL4_NoError) {
        cspace_delete_cap(cur_cspace, _cap);
        ut_free(_memory, seL4_PageTableBits);
        throw std::runtime_error("Failed to map in seL4 page table: " + std::to_string(err));
    }
}

PageTable::~PageTable() {
    _pages.clear();

    assert(seL4_ARM_PageTable_Unmap(_cap) == seL4_NoError);
    assert(cspace_delete_cap(cur_cspace, _cap) == CSPACE_NOERROR);
    ut_free(_memory, seL4_PageTableBits);
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
        throw std::runtime_error("Address is already mapped");
    }
}

void PageTable::unmap(vaddr_t address) {
    _checkAddress(address);

    if (_pages.erase(_toIndex(address)) == 0)
        throw std::out_of_range("Address is not mapped");
}

const MappedPage& PageTable::lookup(vaddr_t address) const {
    _checkAddress(address);

    return _pages.at(_toIndex(address));
}

void PageTable::_checkAddress(vaddr_t address) const {
    if ((address & ~-(1 << seL4_PageBits)) != 0)
        throw std::invalid_argument("Address is not aligned");
    if ((address & _baseAddress) != _baseAddress)
        throw std::invalid_argument("Address does not belong to this page table");
}

////////////////
// MappedPage //
////////////////

MappedPage::MappedPage(Page page, PageDirectory& directory, vaddr_t address, Attributes attributes):
    _isValid(true),
    _page(std::move(page)),
    _address(address),
    _attributes(attributes)
{
    int rights = 0;
    if (attributes.read)
        rights |= seL4_CanRead;
    if (attributes.write)
        rights |= seL4_CanWrite;
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
        throw std::runtime_error("Failed to map in page: " + std::to_string(err));
}

MappedPage::~MappedPage() {
    if (_isValid)
        assert(seL4_ARM_Page_Unmap(_page.getCap()) == seL4_NoError);
}

MappedPage::MappedPage(MappedPage&& other) noexcept:
    _isValid(std::move(other._isValid)),
    _page(std::move(other._page)),
    _address(std::move(other._address)),
    _attributes(std::move(other._attributes))
{
    other._isValid = false;
}

MappedPage& MappedPage::operator=(MappedPage&& other) noexcept {
    _isValid = std::move(other._isValid);
    other._isValid = false;

    _page = std::move(other._page);
    _address = std::move(other._address);
    _attributes = std::move(other._attributes);

    return *this;
}

}
