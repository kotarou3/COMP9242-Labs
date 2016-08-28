#include <algorithm>
#include <limits.h>

#include "internal/memory/UserMemory.h"

namespace memory {

UserMemory::UserMemory(process::Process& process, vaddr_t address):
    _process(process),
    _address(address)
{}

std::vector<uint8_t> UserMemory::read(size_t bytes, bool bypassAttributes) {
    auto map = _mapIn(bytes, Attributes{.read = true}, bypassAttributes);

    std::vector<uint8_t> result(bytes);
    uint8_t* start = reinterpret_cast<uint8_t*>(map.getAddress() + pageOffset(_address));
    std::copy(
        start, start + bytes,
        result.begin()
    );

    return result;
}

std::string UserMemory::readString(size_t max_size, bool bypassAttributes) {
    auto result = read(max_size, bypassAttributes);
    auto end = result.cfind('\0');
    if (end == result.cend())
        throw std::runtime_error("Attempted to map in a string that was either longer than allowed or was not null terminated");
    return std::string(result.cbegin(), end);
}

void UserMemory::write(uint8_t* from, size_t bytes, bool bypassAttributes) {
    auto map = _mapIn(bytes, Attributes{.read = false, .write = true}, bypassAttributes);
    std::copy(
        from, from + bytes,
        reinterpret_cast<uint8_t*>(map.getAddress() + pageOffset(_address))
    );
}

ScopedMapping UserMemory::_mapIn(size_t bytes, Attributes attributes, bool bypassAttributes) {
    size_t startPadding = pageOffset(_address);
    vaddr_t alignedAddress = _address - startPadding;
    size_t pages = numPages(bytes + startPadding);

    // Make sure all the pages are allocated in the target process
    for (size_t p = 0; p < pages; ++p) {
        _process.handlePageFault(
            alignedAddress + p * PAGE_SIZE,
            bypassAttributes ? Attributes{} : attributes
        );
    }

    // Allocate some space in SOS' virtual memory for the target pages
    auto map = process::getSosProcess().maps.insertScoped(
        0, pages,
        attributes, Mapping::Flags{.shared = false}
    );

    // Map in a copy of the process' pages into the SOS process
    for (size_t p = 0; p < pages; ++p) {
        auto page = _process.pageDirectory.lookup(alignedAddress + p * PAGE_SIZE).getPage().copy();
        process::getSosProcess().pageDirectory.map(
            std::move(page), map.getAddress() + p * PAGE_SIZE,
            attributes
        );
    }

    return map;
}

}
