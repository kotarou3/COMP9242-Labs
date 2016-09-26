#include <stdexcept>
#include <system_error>

#include <assert.h>
#include <stdint.h>
#include <sys/mman.h>

#include "internal/memory/Mappings.h"
#include "internal/memory/layout.h"
#include "internal/syscall/mmap.h"

namespace syscall {

async::future<int> brk(process::Process& /*process*/, memory::vaddr_t /*addr*/) {
    // We don't actually implement this - we let malloc() use mmap2() instead
    throw std::system_error(ENOSYS, std::system_category(), "brk() not implemented");
}

async::future<int> mmap2(process::Process& process, memory::vaddr_t addr, size_t length, int prot, int flags, int /*fd*/, off_t /*offset*/) {
    if (memory::pageAlign(addr) != addr || memory::pageAlign(length) != length)
        throw std::invalid_argument("Invalid page or length alignment");

    if (prot & ~(PROT_READ | PROT_WRITE | PROT_EXEC))
        throw std::invalid_argument("Invalid page protection");

    if (flags & ~(MAP_SHARED | MAP_PRIVATE | MAP_ANONYMOUS | MAP_LOCKED | MAP_FIXED))
        throw std::invalid_argument("Invalid flags");
    if ((flags & MAP_SHARED) && (flags & MAP_PRIVATE))
        throw std::invalid_argument("Page cannot be both shared and private");
    if (!(flags & (MAP_SHARED | MAP_PRIVATE)))
        throw std::invalid_argument("Page must be either shared or private");

    if (!(flags & MAP_ANONYMOUS))
        throw std::system_error(ENOSYS, std::system_category(), "Non-anonymous pages are not implemented");

    if (process.isSosProcess) {
        // We need to lock all SOS allocations, since we can't handle page faults
        // on ourself
        flags |= MAP_LOCKED;
    }

    auto map = std::make_shared<memory::ScopedMapping>(process.maps.insert(
        addr, memory::numPages(length),
        memory::Attributes{
            .read = prot & PROT_READ,
            .write = prot & PROT_WRITE,
            .execute = prot & PROT_EXEC,
            .locked = flags & MAP_LOCKED
        },
        memory::Mapping::Flags{
            .shared = flags & MAP_SHARED,
            .fixed = flags & MAP_FIXED
        }
    ));

    async::future<void> future;
    if (flags & MAP_LOCKED) {
        future = process.pageFaultMultiple(map->getStart(), map->getPages(), memory::Attributes{}, map);
    } else {
        async::promise<void> promise;
        promise.set_value();
        future = promise.get_future();
    }

    return future.then([map](async::future<void> result) {
        result.get();

        int _result = static_cast<int>(map->getStart());
        map->release();
        return _result;
    });
}

async::future<int> munmap(process::Process& process, memory::vaddr_t addr, size_t length) {
    if (memory::pageAlign(addr) != addr || memory::pageAlign(length) != length)
        throw std::invalid_argument("Invalid page or length alignment");

    process.maps.erase(addr, length / PAGE_SIZE);
    return async::make_ready_future(0);
}

}

extern "C" int sys_brk(va_list ap) {
    // Use an static allocation to allow malloc() before the memory subsystem
    // is ready for mmap()s
    static uint8_t brkArea[memory::SOS_INIT_AREA_SIZE] [[gnu::section(".sos_brk_area")]];
    static uint8_t* brk = brkArea;
    static uint8_t* brkEnd = &brkArea[memory::SOS_INIT_AREA_SIZE];
    static bool isUpdatingBrk;

    assert(reinterpret_cast<memory::vaddr_t>(&brkArea) == memory::SOS_BRK_START);

    if (brkEnd == &brkArea[memory::SOS_INIT_AREA_SIZE] && !isUpdatingBrk && memory::isReady()) {
        isUpdatingBrk = true;

        // We can mmap() now, so let's expand our brk to its full size
        assert(mmap(
            brkEnd,
            memory::SOS_BRK_END - reinterpret_cast<memory::vaddr_t>(brkEnd),
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,
            0, 0
        ) == brkEnd);

        brkEnd = reinterpret_cast<uint8_t*>(memory::SOS_BRK_END);
    }

    uint8_t* newbrk = va_arg(ap, uint8_t*);
    if (brkArea <= newbrk && newbrk <= brkEnd)
        return reinterpret_cast<int>(brk = newbrk);
    else
        return reinterpret_cast<int>(brk);
}

extern "C" int sys_mremap() {
    assert(!"Not implemented");
    __builtin_unreachable();
}

extern "C" int sys_madvise() {
    // free() needs this
    return -ENOSYS;
}

extern "C" int sys_mmap2(va_list ap) {
    // It's possible for us to be called recursively, so make sure it doesn't
    // get out of control
    static size_t _recursionDepth;
    static bool _isInOverflowMode;
    if (_isInOverflowMode || _recursionDepth > 2) {
        _isInOverflowMode = _recursionDepth > 0;
        return -ENOMEM;
    }
    ++_recursionDepth;

    constexpr const size_t argc = 6;
    seL4_Word argv[argc];
    for (size_t a = 0; a < argc; ++a)
        argv[a] = va_arg(ap, seL4_Word);

    try {
        auto result = syscall::handle(process::getSosProcess(), SYS_mmap2, argc, argv);
        assert(result.is_ready());

        --_recursionDepth;
        return result.get();
    } catch (...) {
        --_recursionDepth;
        return -syscall::exceptionToErrno(std::current_exception());
    }
}

FORWARD_SYSCALL(munmap, 2);
