#include <algorithm>
#include <system_error>
#include <limits.h>

#include "internal/memory/UserMemory.h"

namespace memory {

UserMemory::UserMemory(std::weak_ptr<process::Process> process, vaddr_t address):
    _process(process),
    _address(address)
{}

async::future<std::string> UserMemory::readString(bool bypassAttributes) {
    // Map in one page
    return mapIn<char>(1, Attributes{.read = true}, bypassAttributes)
        .then([_process = _process, _address = _address, bypassAttributes](auto map) {
            auto map_ = std::move(map.get());

            // Read until end of string or page
            std::string string;
            char* pageEnd = reinterpret_cast<char*>(map_.second.getEnd());
            for (char* c = map_.first; c < pageEnd; ++c)
                if (*c)
                    string.push_back(*c);
                else
                    return async::make_ready_future(string);

            // Continue reading from next page
            return UserMemory(_process, _address + pageEnd - map_.first)
                .readString(bypassAttributes)
                .then([string](auto result) {
                    return string + result.get();
                });
        });
}

async::future<std::pair<uint8_t*, ScopedMapping>> UserMemory::_mapIn(size_t bytes, Attributes attributes, bool bypassAttributes) try {
    size_t startPadding = pageOffset(_address);
    vaddr_t alignedAddress = _address - startPadding;
    size_t pages = numPages(bytes + startPadding);

    std::shared_ptr<process::Process> process(_process);
    if (process->isSosProcess) {
        // XXX: SOS currently has inaccurate mappings for itself, so we can't
        // check the address range attributes. For now, we just assume they're
        // read+write
        process->maps.lookup(alignedAddress);

        return async::make_ready_future(std::make_pair(reinterpret_cast<uint8_t*>(_address), ScopedMapping()));
    } else {
        // Allocate some space in SOS' virtual memory for the target pages
        attributes.locked = true;
        auto map = std::make_shared<ScopedMapping>(process::getSosProcess()->maps.insert(
            0, pages,
            attributes,
            Mapping::Flags{.shared = false}
        ));

        async::promise<void> promise;
        promise.set_value();
        auto future = promise.get_future();

        for (size_t p = 0; p < pages; ++p) {
            vaddr_t srcAddr = alignedAddress + p * PAGE_SIZE;
            vaddr_t destAddr = map->getAddress() + p * PAGE_SIZE;
            Attributes attr = bypassAttributes ? Attributes{} : attributes;

            future = future.then([process, srcAddr, attr, map](async::future<void> result) {
                result.get();

                // Make sure the page is allocated in the target process
                return process->handlePageFault(srcAddr, attr);
            }).unwrap().then([process, srcAddr, destAddr, attributes, map](async::future<void> result) {
                result.get();

                // Map in a copy of the process' page into the SOS process
                process::getSosProcess()->pageDirectory.map(
                    process->pageDirectory.lookup(srcAddr)->getPage().copy(),
                    destAddr, attributes
                );
            });
        }

        uint8_t* resultAddress = reinterpret_cast<uint8_t*>(map->getAddress() + startPadding);
        return future.then([resultAddress, map](async::future<void> result) {
            try {
                result.get();
            } catch (const std::invalid_argument& e) {
                std::throw_with_nested(std::system_error(EFAULT, std::system_category(), "Failed to map in user memory"));
            }

            return std::make_pair(resultAddress, std::move(*map));
        });
    }
} catch (const std::invalid_argument& e) {
    std::throw_with_nested(std::system_error(EFAULT, std::system_category(), "Failed to map in user memory"));
}

}
