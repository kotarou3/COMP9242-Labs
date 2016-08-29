#include <algorithm>
#include <limits.h>

#include "internal/memory/UserMemory.h"

namespace memory {

UserMemory::UserMemory(process::Process& process, vaddr_t address):
    _process(process),
    _address(address)
{}

void UserMemory::read(uint8_t* to, size_t bytes, bool bypassAttributes) {
    auto map = _mapIn(bytes, Attributes{.read = true}, bypassAttributes);
    std::copy(map.first, map.first + bytes, to);
}

std::string UserMemory::readString(bool bypassAttributes) {
    std::string result;

    vaddr_t pageEnd;
    {
        // Start reading the string from the first page
        auto map = mapIn<char>(1, Attributes{.read = true}, bypassAttributes);
        pageEnd = map.second.getEnd();
        for (char* c = map.first; c < reinterpret_cast<char*>(pageEnd); ++c)
            if (*c)
                result.push_back(*c);
            else
                goto finished;
    }

    // XXX: Continue reading the next page recursively
    result += UserMemory(_process, pageEnd).readString(bypassAttributes);

finished:
    return result;
}

void UserMemory::write(const uint8_t* from, size_t bytes, bool bypassAttributes) {
    auto map = _mapIn(bytes, Attributes{.read = false, .write = true}, bypassAttributes);
    std::copy(from, from + bytes, map.first);
}

std::pair<uint8_t*, ScopedMapping> UserMemory::_mapIn(size_t bytes, Attributes attributes, bool bypassAttributes) {
    size_t startPadding = pageOffset(_address);
    vaddr_t alignedAddress = _address - startPadding;
    size_t pages = numPages(bytes + startPadding);

    if (_process.isSosProcess) {
        // XXX: SOS currently has inaccurate mappings for itself, so we can't
        // check the address range attributes. For now, we just assume they're
        // read+write, but might not be faulted in
        auto map = _process.maps.lookup(alignedAddress);
        if (!map.flags.reserved) {
            for (size_t p = 0; p < pages; ++p) {
                _process.handlePageFault(
                    alignedAddress + p * PAGE_SIZE,
                    Attributes{}
                );
            }
        }

        return std::make_pair(reinterpret_cast<uint8_t*>(_address), ScopedMapping());
    } else {
        // Make sure all the pages are allocated in the target process
        for (size_t p = 0; p < pages; ++p) {
            _process.handlePageFault(
                alignedAddress + p * PAGE_SIZE,
                bypassAttributes ? Attributes{} : attributes
            );
        }

        // Allocate some space in SOS' virtual memory for the target pages
        auto map = process::getSosProcess().maps.insert(
            0, pages,
            attributes, Mapping::Flags{.shared = false}
        );

        // Map in a copy of the process' pages into the SOS process
        for (size_t p = 0; p < pages; ++p) {
            auto page = _process.pageDirectory.lookup(alignedAddress + p * PAGE_SIZE)->getPage().copy();
            process::getSosProcess().pageDirectory.map(
                std::move(page), map.getAddress() + p * PAGE_SIZE,
                attributes
            );
        }

        return std::make_pair(reinterpret_cast<uint8_t*>(map.getAddress() + startPadding), std::move(map));
    }
}

}
