#pragma once

#include <unordered_map>
#include <utility>
#include <vector>

extern "C" {
    #include <sel4/types.h>
}

#include "internal/async.h"
#include "internal/memory/FrameTable.h"
#include "internal/memory/Page.h"
#include "internal/Capability.h"

namespace process {
    class Process;
}

namespace memory {

using vaddr_t = size_t;

struct Attributes {
    bool read:1;
    bool write:1;
    bool execute:1;
    bool locked:1;
    bool notCacheable:1;
};

constexpr bool operator==(const Attributes& a, const Attributes& b) {
    return std::tie(a.read, a.write, a.execute, a.notCacheable) == std::tie(b.read, b.write, b.execute, b.notCacheable);
}
constexpr bool operator!=(const Attributes& a, const Attributes& b) {
    return !(a == b);
}

class PageTable;
class MappedPage;
class Mapping;

class PageDirectory {
    public:
        explicit PageDirectory(process::Process& process);
        PageDirectory(process::Process& process, seL4_ARM_PageDirectory cap);
        ~PageDirectory();

        PageDirectory(const PageDirectory&) = delete;
        PageDirectory& operator=(const PageDirectory&) = delete;

        PageDirectory(PageDirectory&&) = delete;
        PageDirectory& operator=(PageDirectory&&) = delete;

        size_t countPages() const noexcept;

        void reservePages(vaddr_t from, vaddr_t to);

        // Warning: Returned MappedPage reference is invalidated after another mapping

        async::future<const MappedPage&> makeResident(vaddr_t address, const Mapping& map);
        async::future<const MappedPage&> allocateAndMap(vaddr_t address, Mapping map);

        const MappedPage& map(Page page, vaddr_t address, Attributes attributes);
        void unmap(vaddr_t address) noexcept;
        void clear() noexcept;

        const MappedPage* lookup(vaddr_t address, bool noThrow = false) const;

        seL4_ARM_PageDirectory getCap() const noexcept {return _cap.get();}

    private:
        constexpr static vaddr_t _toIndex(vaddr_t address) noexcept {
            return pageTableAlign(address);
        }

        process::Process& _process;

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
        PageTable& operator=(PageTable&& other) = delete;

        size_t countPages() const noexcept;

        void reservePages();

        const MappedPage& map(Page page, vaddr_t address, Attributes attributes);
        void unmap(vaddr_t address) noexcept;

        MappedPage* lookup(vaddr_t address, bool noThrow = false);
        const MappedPage* lookup(vaddr_t address, bool noThrow = false) const;

        seL4_ARM_PageTable getCap() const noexcept {return _cap.get();};

    private:
        void _checkAddress(vaddr_t address) const;
        constexpr static vaddr_t _toIndex(vaddr_t address) noexcept {
            return pageAlign(pageTableOffset(address));
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
        MappedPage& operator=(MappedPage&& other) = delete;

        void enableReference(PageDirectory& directory);
        async::future<void> swapIn();

        const Page& getPage() const noexcept {return _page;}
        vaddr_t getAddress() const noexcept {return _address;}
        Attributes getAttributes() const noexcept {return _attributes;}

        seL4_CapRights seL4Rights() const;
        seL4_ARM_VMAttributes seL4Attributes() const;

    private:
        Page _page;
        vaddr_t _address;
        Attributes _attributes;

        friend void FrameTable::init(paddr_t start, paddr_t end);
};

}
