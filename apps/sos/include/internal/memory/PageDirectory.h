#pragma once

#include <unordered_map>
#include <vector>

#include "internal/memory/FrameTable.h"
#include "internal/Capability.h"

extern "C" {
    #include <sel4/types.h>
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
        PageDirectory() = default;
        explicit PageDirectory(seL4_ARM_PageDirectory cap);
        ~PageDirectory();

        PageDirectory(const PageDirectory&) = delete;
        PageDirectory& operator=(const PageDirectory&) = delete;

        PageDirectory(PageDirectory&&) = delete;
        PageDirectory& operator=(PageDirectory&&) = delete;

        // Warning: Returned reference is invalidated after another mapping
        const MappedPage& allocateAndMap(vaddr_t address, Attributes attributes);
        const MappedPage& map(Page page, vaddr_t address, Attributes attributes);
        void unmap(vaddr_t address);
        const MappedPage* lookup(vaddr_t address, bool noThrow = false) const;

        seL4_ARM_PageDirectory getCap() const noexcept {return _cap.get();}

    private:
        constexpr static vaddr_t _toIndex(vaddr_t address) noexcept {
            return address & -((1 << (seL4_PageTableBits + seL4_PageBits)) / sizeof(seL4_Word));
        }

        Capability<seL4_ARM_PageDirectoryObject, seL4_PageDirBits> _cap;
        std::unordered_map<vaddr_t, PageTable> _tables;
};

class PageTable {
    public:
        PageTable(PageDirectory& parent, vaddr_t baseAddress);
        ~PageTable();

        PageTable(const PageTable&) = delete;
        PageTable& operator=(const PageTable&) = delete;

        PageTable(PageTable&& other) = default;
        PageTable& operator=(PageTable&& other) = default;

        const MappedPage& map(Page page, vaddr_t address, Attributes attributes);
        void unmap(vaddr_t address);
        const MappedPage* lookup(vaddr_t address, bool noThrow = false) const;

        seL4_ARM_PageTable getCap() const noexcept {return _cap.get();};

    private:
        void _checkAddress(vaddr_t address) const;
        constexpr static vaddr_t _toIndex(vaddr_t address) noexcept {
            return pageAlign(address) & ~-((1 << (seL4_PageTableBits + seL4_PageBits)) / sizeof(seL4_Word));
        }

        PageDirectory& _parent;
        vaddr_t _baseAddress;

        Capability<seL4_ARM_PageTableObject, seL4_PageTableBits> _cap;
        std::unordered_map<vaddr_t, MappedPage> _pages;
};

class MappedPage {
    public:
        MappedPage(Page page, PageDirectory& directory, vaddr_t address, Attributes attributes);
        ~MappedPage();

        MappedPage(const MappedPage&) = delete;
        MappedPage& operator=(const MappedPage&) = delete;

        MappedPage(MappedPage&& other) = default;
        MappedPage& operator=(MappedPage&& other) = default;

        const Page& getPage() const noexcept {return _page;}
        vaddr_t getAddress() const noexcept {return _address;}
        Attributes getAttributes() const noexcept {return _attributes;}
        seL4_CapRights seL4Rights();
        seL4_ARM_VMAttributes seL4Attributes();
    private:
        Page _page;
        vaddr_t _address;
        Attributes _attributes;

        friend void FrameTable::init(paddr_t start, paddr_t end);
};

}
