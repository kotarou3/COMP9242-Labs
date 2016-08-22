#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/mman.h>

#include "internal/memory/Mappings.h"
#include "internal/process/Thread.h"

namespace syscall {

int brk(process::Process& process, memory::vaddr_t addr) noexcept {
    // We don't actually implement this - we let malloc() use mmap2() instead
    return -ENOSYS;
}

int mmap2(process::Process& process, memory::vaddr_t addr, size_t length, int prot, int flags, int fd, off_t offset) noexcept {
    if (memory::pageAlign(addr) != addr || memory::pageAlign(length) != length)
        return -EINVAL;

    if (prot & ~(PROT_READ | PROT_WRITE | PROT_EXEC))
        return -EINVAL;

    if (flags & ~(MAP_SHARED | MAP_PRIVATE | MAP_ANONYMOUS))
        return -ENOSYS;
    if ((flags & MAP_SHARED) && (flags & MAP_PRIVATE))
        return -EINVAL;
    if (!(flags & (MAP_SHARED | MAP_PRIVATE)))
        return -EINVAL;

    if (!(flags & MAP_ANONYMOUS))
        return -ENOSYS;

    try {
        auto map = process.maps.insertScoped(
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
        return result;
    } catch (const std::bad_alloc&) {
        return -ENOMEM;
    } catch (...) {
        return -EINVAL;
    }
}

int munmap(process::Process& process, memory::vaddr_t addr, size_t length) noexcept {
    if (memory::pageAlign(addr) != addr || memory::pageAlign(length) != length)
        return -EINVAL;

    try {
        process.maps.erase(addr, length / PAGE_SIZE);
        return 0;
    } catch (...) {
        return -EINVAL;
    }
}

}

extern "C" {

int sys_brk(va_list ap) {
    // Use an static allocation to allow malloc() before the memory subsystem
    // is ready for mmap()s
    constexpr const size_t SOS_PROCESS_INIT_SIZE = 0x100000;
    static uint8_t initArea[SOS_PROCESS_INIT_SIZE];
    static uint8_t* brk = initArea;

    uint8_t* newbrk = va_arg(ap, uint8_t*);
    if (initArea <= newbrk && newbrk <= &initArea[SOS_PROCESS_INIT_SIZE])
        return reinterpret_cast<int>(brk = newbrk);
    else
        return reinterpret_cast<int>(brk);
}

int sys_mmap2(va_list ap) {
    memory::vaddr_t addr = va_arg(ap, memory::vaddr_t);
    size_t length = va_arg(ap, size_t);
    int prot = va_arg(ap, int);
    int flags = va_arg(ap, int);
    int fd = va_arg(ap, int);
    off_t offset = va_arg(ap, off_t);

    return syscall::mmap2(process::getSosProcess(), addr, length, prot, flags, fd, offset);
}

int sys_munmap(va_list ap) {
    memory::vaddr_t addr = va_arg(ap, memory::vaddr_t);
    size_t length = va_arg(ap, size_t);

    return syscall::munmap(process::getSosProcess(), addr, length);
}

int sys_mremap(va_list ap) {
    assert(!"Not implemented");
    return -ENOSYS;
}

}
