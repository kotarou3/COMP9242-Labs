#pragma once

#include <functional>
#include <map>
#include <memory>
#include <vector>

#include "internal/memory/PageDirectory.h"

namespace process {
    class Process;
    std::shared_ptr<Process> getSosProcess() noexcept;
}

namespace memory {

struct Mapping {
    vaddr_t start;
    vaddr_t end;

    Attributes attributes;
    struct Flags {
        bool shared:1;
        bool fixed:1; // Does not automatically unmap!
        bool stack:1;
        bool reserved:1; // Never map in this mapping
    } flags;
};
class ScopedMapping;

class Mappings {
    public:
        Mappings(const Mappings&) = delete;
        Mappings& operator=(const Mappings&) = delete;

        ScopedMapping insert(vaddr_t address, size_t pages, Attributes attributes, Mapping::Flags flags);
        void erase(vaddr_t address, size_t pages);

        const Mapping& lookup(vaddr_t address) const;

    private:
        Mappings() = default;

        static bool _isOverflowing(vaddr_t address, size_t pages) noexcept;
        static void _checkAddress(vaddr_t address, size_t pages);

        const Mapping* _findFirstOverlap(vaddr_t address, size_t pages) const noexcept;
        Mapping* _findFirstOverlap(vaddr_t address, size_t pages) noexcept;
        bool _isOverlapping(vaddr_t address, size_t pages) const noexcept;

        enum class OverlapType { None, Complete, Start, Middle, End };
        static OverlapType _classifyOverlap(vaddr_t address, size_t pages, const Mapping& map) noexcept;

        std::weak_ptr<process::Process> _process;

        std::map<vaddr_t, Mapping> _maps;

        friend class ScopedMapping;

        friend class process::Process;
        friend std::shared_ptr<process::Process> process::getSosProcess() noexcept;
};

class ScopedMapping {
    public:
        ScopedMapping() = default;
        ScopedMapping(Mappings& maps, vaddr_t address, size_t pages);
        ~ScopedMapping();

        ScopedMapping(const ScopedMapping&) = delete;
        ScopedMapping& operator=(const ScopedMapping&) = delete;

        ScopedMapping(ScopedMapping&& other) = default;
        ScopedMapping& operator=(ScopedMapping&& other) = default;

        void release() noexcept;

        vaddr_t getAddress() const noexcept {return _address;};
        size_t getPages() const noexcept {return _pages;};

        vaddr_t getStart() const noexcept {return _address;};
        vaddr_t getEnd() const noexcept {return _address + _pages * PAGE_SIZE;};

    private:
        std::shared_ptr<process::Process> _process;
        vaddr_t _address;
        size_t _pages;
};

}
