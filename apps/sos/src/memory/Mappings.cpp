#include <stdexcept>
#include <system_error>

#include <assert.h>
#include <limits.h>

extern "C" {
    #include <sel4/types.h>
}

#include "internal/memory/Mappings.h"
#include "internal/memory/layout.h"
#include "internal/process/Process.h"
#include "internal/RandomDevice.h"

namespace memory {

//////////////
// Mappings //
//////////////

Mappings::Mappings(process::Process& process):
    _process(process)
{}

ScopedMapping Mappings::insert(vaddr_t address, size_t pages, Attributes attributes, Mapping::Flags flags, std::shared_ptr<fs::File> file, off64_t fileOffset, size_t fileSize, size_t memoryOffset) {
    _checkAddress(address, pages);

    if (flags.shared)
        throw std::system_error(ENOSYS, std::system_category(), "Shared memory not implemented yet");

    if (flags.stack)
        ++pages; // Add space for guard page

    if (_isOverlapping(address, pages) || (!address && !flags.fixed)) {
        if (flags.fixed)
            throw std::invalid_argument("Fixed mapping overlaps with existing mappings");

        // A buddy-like allocator would be better, but this will do for our toy OS
        const vaddr_t mmapStart = flags.stack ? MMAP_STACK_START : MMAP_START;
        const vaddr_t mmapEnd = flags.stack ? MMAP_STACK_END : MMAP_END;

        // Try to pick a random address first
        std::uniform_int_distribution<vaddr_t> addressDist(mmapStart / PAGE_SIZE, mmapEnd / PAGE_SIZE);
        for (size_t n = 0; n < MMAP_RAND_ATTEMPTS; ++n) {
            address = addressDist(RandomDevice::getSingleton()) * PAGE_SIZE;
            if (!_isOverlapping(address, pages))
                goto haveValidAddress;
        }

        // Fall back to linear scan
        address = mmapStart;
        do {
            if (_isOverflowing(address, pages))
                break;

            Mapping* overlap = _findFirstOverlap(address, pages);
            if (!overlap) {
                if (_isOverlapping(address, pages))
                    break; // Overlaps with kernel
                goto haveValidAddress;
            }

            address = overlap->end;
        } while (mmapStart < address && address < mmapEnd);

        throw std::system_error(ENOMEM, std::system_category(), "Could not find an empty mapping with enough space");
    }

haveValidAddress:

    auto map = _maps[address] = Mapping{
        .start = address,
        .end = address + pages * PAGE_SIZE,
        .attributes = attributes,
        .flags = flags,

        .file = file,
        .fileOffset = fileOffset,
        .fileSize = fileSize,
        .memoryOffset = memoryOffset
    };
    return ScopedMapping(*this, map.start, pages);
}

void Mappings::erase(vaddr_t address, size_t pages) {
    _checkAddress(address, pages);

    vaddr_t end = address + pages * PAGE_SIZE;

    while (Mapping* overlap = _findFirstOverlap(address, pages)) {
        vaddr_t unmapStart;
        size_t unmapPages;

        switch (_classifyOverlap(address, pages, *overlap)) {
            case OverlapType::Complete:
                unmapStart = overlap->start;
                unmapPages = numPages(overlap->end - overlap->start);

                _maps.erase(overlap->start);
                break;

            case OverlapType::Start: {
                unmapStart = overlap->start;
                unmapPages = numPages(end - overlap->start);

                Mapping endPart = std::move(*overlap);
                _maps.erase(endPart.start);

                endPart.start = end;
                _maps[endPart.start] = std::move(endPart);

                break;
            }

            case OverlapType::Middle: {
                unmapStart = address;
                unmapPages = pages;

                overlap->end = address;

                Mapping endPart = *overlap;
                endPart.start = end;
                _maps[endPart.start] = std::move(endPart);

                break;
            }

            case OverlapType::End:
                unmapStart = address;
                unmapPages = numPages(overlap->end - address);

                overlap->end = address;
                break;

            case OverlapType::None:
            default:
                assert(false);
                __builtin_unreachable();
        }

        for (size_t p = 0; p < unmapPages; ++p) {
            _process.pageDirectory.unmap(unmapStart);
            unmapStart += PAGE_SIZE;
        }
    }
}

void Mappings::clear() noexcept {
    erase(0, numPages(MMAP_STACK_END));
    _maps.clear();
}

const Mapping& Mappings::lookup(vaddr_t address) const {
    _checkAddress(address, 1);

    const Mapping* overlap = _findFirstOverlap(address, 1);
    if (!overlap)
        throw std::invalid_argument("Address not mapped");

    return *overlap;
}

bool Mappings::_isOverflowing(vaddr_t address, size_t pages) noexcept {
    size_t maxPages;
    if (address)
        maxPages = -address / PAGE_SIZE;
    else
        maxPages = 1 << (sizeof(size_t) * CHAR_BIT - seL4_PageBits);

    return pages > maxPages;
}

void Mappings::_checkAddress(vaddr_t address, size_t pages) {
    if (pageAlign(address) != address)
        throw std::invalid_argument("Address is not aligned");

    if (pages == 0)
        throw std::invalid_argument("Must specify at least one page");
    if (_isOverflowing(address, pages))
        throw std::invalid_argument("Too many pages");
}

const Mapping* Mappings::_findFirstOverlap(vaddr_t address, size_t pages) const noexcept {
    vaddr_t end = address + pages * PAGE_SIZE;

    auto it = _maps.upper_bound(address);
    if (it != _maps.begin()) {
        // Check if the first mapping starting at <= `address` overlaps
        auto prev = it;
        --prev;
        if (prev->second.start < end && address < prev->second.end)
            return &prev->second;
    }

    // Check if the first mapping starting at > `address` overlaps
    if (it != _maps.end() && it->second.start < end && address < it->second.end)
        return &it->second;

    return nullptr;
}

Mapping* Mappings::_findFirstOverlap(vaddr_t address, size_t pages) noexcept {
    return const_cast<Mapping*>(const_cast<const Mappings*>(this)->_findFirstOverlap(address, pages));
}

bool Mappings::_isOverlapping(vaddr_t address, size_t pages) const noexcept {
    if (_isOverflowing(address, pages))
        return true;
    if (address >= KERNEL_START || address + pages * PAGE_SIZE > KERNEL_START)
        return true;
    return _findFirstOverlap(address, pages);
}

Mappings::OverlapType Mappings::_classifyOverlap(vaddr_t address, size_t pages, const Mapping& map) noexcept {
    vaddr_t end = address + pages * PAGE_SIZE;
    assert(address <= end && map.start <= map.end);

    bool startsBefore = address <= map.start;
    bool startsInside = map.start < address && address < map.end;
    bool endsAfter = map.end <= end;
    bool endsInside = map.start < end && end < map.end;

    if (startsBefore && endsAfter)
        return OverlapType::Complete;
    else if (startsBefore && endsInside)
        return OverlapType::Start;
    else if (startsInside && endsInside)
        return OverlapType::Middle;
    else if (startsInside && endsAfter)
        return OverlapType::End;
    else
        return OverlapType::None;
}

///////////////////
// ScopedMapping //
///////////////////

ScopedMapping::ScopedMapping(Mappings& maps, vaddr_t address, size_t pages):
    _process(maps._process.shared_from_this()),
    _address(address),
    _pages(pages)
{}

ScopedMapping::ScopedMapping(vaddr_t address, size_t pages):
    _address(address),
    _pages(pages)
{}

ScopedMapping::~ScopedMapping() {
    reset();
}

void ScopedMapping::release() noexcept {
    _process.reset();
}

void ScopedMapping::reset() noexcept {
    if (_process) {
        _process->maps.erase(_address, _pages);
        _process.reset();
    }
}

}
