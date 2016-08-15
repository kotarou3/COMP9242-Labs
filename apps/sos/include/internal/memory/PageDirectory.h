#pragma once

#include <unordered_map>
#include <vector>

#include "internal/memory/FrameTable.h"

extern "C" {
    #include <sel4/sel4.h>
}

namespace memory {

using vaddr_t = size_t;

struct Attributes {
    bool read:1;
    bool write:1;
    bool execute:1;
    bool notCacheable:1;
};

class PageTable;
class MappedPage;

class PageDirectory {
    public:
        explicit PageDirectory(seL4_ARM_PageDirectory cap);

        std::vector<std::reference_wrapper<const MappedPage>> allocateAndMap(size_t pages, vaddr_t address, Attributes attributes);
        const MappedPage& map(Page page, vaddr_t address, Attributes attributes);
        void unmap(vaddr_t address);
        const MappedPage& lookup(vaddr_t address) const;

        seL4_ARM_PageDirectory getCap() const noexcept {return _cap;}

    private:
        constexpr static vaddr_t _toIndex(vaddr_t address) noexcept {
            return address & -((1 << (seL4_PageTableBits + seL4_PageBits)) / sizeof(seL4_Word));
        }

        seL4_ARM_PageDirectory _cap;
        std::unordered_map<vaddr_t, PageTable> _tables;
};

class PageTable {
    public:
        PageTable(PageDirectory& parent, vaddr_t baseAddress);
        ~PageTable();

        const MappedPage& map(Page page, vaddr_t address, Attributes attributes);
        void unmap(vaddr_t address);
        const MappedPage& lookup(vaddr_t address) const;

        seL4_ARM_PageTable getCap() const noexcept {return _cap;};

    private:
        void _checkAddress(vaddr_t address) const;
        constexpr static vaddr_t _toIndex(vaddr_t address) noexcept {
            return address & ~-((1 << (seL4_PageTableBits + seL4_PageBits)) / sizeof(seL4_Word)) & -(1 << seL4_PageBits);
        }

        PageDirectory& _parent;
        vaddr_t _baseAddress;

        seL4_Word _memory;
        seL4_ARM_PageTable _cap;

        std::unordered_map<vaddr_t, MappedPage> _pages;
};

class MappedPage {
    public:
        MappedPage(Page page, PageDirectory& directory, vaddr_t address, Attributes attributes);
        ~MappedPage();

        MappedPage(const MappedPage&) = delete;
        MappedPage& operator=(const MappedPage&) = delete;

        MappedPage(MappedPage&& other) noexcept;
        MappedPage& operator=(MappedPage&& other) noexcept;

        //Page getPage() const noexcept {return _page;}
        vaddr_t getAddress() const noexcept {return _address;}
        Attributes getAttributes() const noexcept {return _attributes;}

    private:
        bool _isValid; // To allow move semantics

        Page _page;
        vaddr_t _address;
        Attributes _attributes;
};

extern PageDirectory sosPageDirectory;

}
