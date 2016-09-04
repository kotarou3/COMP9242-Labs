#include <algorithm>
#include <system_error>
#include <limits.h>

#include "internal/memory/UserMemory.h"
#include "../../include/internal/memory/UserMemory.h"
#include <boost/format.hpp>

namespace memory {

UserMemory::UserMemory(process::Process& process, vaddr_t address):
    _process(process),
    _address(address)
{}

std::string UserMemory::readString(bool bypassAttributes) {
    std::string result;

    vaddr_t currentAddress = _address;
    while (true) {
        // Map in one page
        auto map = UserMemory(_process, currentAddress).mapIn<char>(1, Attributes{.read = true}, bypassAttributes);
        vaddr_t pageEnd = map.second.getEnd();

        // Read until end of string or page
        for (char* c = map.first; c < reinterpret_cast<char*>(pageEnd); ++c)
            if (*c)
                result.push_back(*c);
            else
                goto finished;

        // Continue reading from next page
        currentAddress = pageEnd;
    }

finished:
    return result;
}

std::pair<uint8_t*, ScopedMapping> UserMemory::_mapIn(size_t bytes, Attributes attributes, bool bypassAttributes) try {
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
} catch (const std::invalid_argument& e) {
    printf("User memory was %p\n", (void*)_address);
    std::throw_with_nested(std::system_error(EFAULT, std::system_category(), "Failed to map in user memory"));
}

}
