#include <stdexcept>
#include <system_error>

#include <assert.h>
#include <stdint.h>
#include <sys/mman.h>

#include "internal/memory/Mappings.h"
#include "internal/syscall/mmap.h"

namespace syscall {

boost::future<int> brk(process::Process& /*process*/, memory::vaddr_t /*addr*/) {
    // We don't actually implement this - we let malloc() use mmap2() instead
    throw std::system_error(ENOSYS, std::system_category(), "brk() not implemented");
}

boost::future<int> mmap2(process::Process& process, memory::vaddr_t addr, size_t length, int prot, int flags, int /*fd*/, off_t /*offset*/) {
    if (memory::pageAlign(addr) != addr || memory::pageAlign(length) != length)
        throw std::invalid_argument("Invalid page or length alignment");

    if (prot & ~(PROT_READ | PROT_WRITE | PROT_EXEC))
        throw std::invalid_argument("Invalid page protection");

    if (flags & ~(MAP_SHARED | MAP_PRIVATE | MAP_ANONYMOUS))
        throw std::invalid_argument("Invalid flags");
    if ((flags & MAP_SHARED) && (flags & MAP_PRIVATE))
        throw std::invalid_argument("Page cannot be both shared and private");
    if (!(flags & (MAP_SHARED | MAP_PRIVATE)))
        throw std::invalid_argument("Page must be either shared or private");

    if (!(flags & MAP_ANONYMOUS))
        throw std::system_error(ENOSYS, std::system_category(), "Non-anonymous pages are not implemented");

    auto map = process.maps.insert(
        addr, length / PAGE_SIZE,
        memory::Attributes{
            .read = prot & PROT_READ,
            .write = prot & PROT_WRITE,
            .execute = prot & PROT_EXEC
        },
        memory::Mapping::Flags{
            .shared = flags & MAP_SHARED
        }
    );

    if (process.isSosProcess) {
        // We need to fault in all the pages if the allocation came from
        // ourselves, since we can't handle page faults on ourself
        for (addr = map.getStart(); addr < map.getEnd(); addr += PAGE_SIZE)
            process.handlePageFault(addr, memory::Attributes{});
    }

    int result = static_cast<int>(map.getStart());
    map.release();
    return boost::make_ready_future(result);
}

boost::future<int> munmap(process::Process& process, memory::vaddr_t addr, size_t length) {
    if (memory::pageAlign(addr) != addr || memory::pageAlign(length) != length)
        throw std::invalid_argument("Invalid page or length alignment");

    process.maps.erase(addr, length / PAGE_SIZE);
    return boost::make_ready_future(0);
}

}

extern "C" int sys_brk(va_list ap) {
    // Use an static allocation to allow malloc() before the memory subsystem
    // is ready for mmap()s
    constexpr const size_t SOS_PROCESS_INIT_SIZE = 0x800000;
    static uint8_t initArea[SOS_PROCESS_INIT_SIZE];
    static uint8_t* brk = initArea;

    uint8_t* newbrk = va_arg(ap, uint8_t*);
    if (initArea <= newbrk && newbrk <= &initArea[SOS_PROCESS_INIT_SIZE])
        return reinterpret_cast<int>(brk = newbrk);
    else
        return reinterpret_cast<int>(brk);
}

extern "C" int sys_mremap() {
    assert(!"Not implemented");
    __builtin_unreachable();
}

FORWARD_SYSCALL(mmap2, 6);
FORWARD_SYSCALL(munmap, 2);
