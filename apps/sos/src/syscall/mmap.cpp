#include <stdexcept>
#include <system_error>

#include <assert.h>
#include <stdint.h>
#include <sys/mman.h>

#include "internal/memory/Mappings.h"
#include "internal/memory/layout.h"
#include "internal/syscall/mmap.h"

namespace syscall {

async::future<int> brk(std::weak_ptr<process::Process> /*process*/, memory::vaddr_t /*addr*/) {
    // We don't actually implement brk for user level - we let malloc() use mmap2() instead
    throw std::system_error(ENOSYS, std::system_category(), "brk() not implemented");
}

async::future<int> mmap2(std::weak_ptr<process::Process> process, memory::vaddr_t addr, size_t length, int prot, int flags, int /*fd*/, int /*offset*/) {
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

    std::shared_ptr<process::Process> _process(process);
    if (_process->isSosProcess) {
        // We need to lock all SOS allocations, since we can't handle page faults
        // on ourself
        flags |= MAP_LOCKED;
    }

    auto map = std::make_shared<memory::ScopedMapping>(_process->maps.insert(
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
        future = _process->pageFaultMultiple(map->getStart(), map->getPages(), memory::Attributes{}, map);
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

async::future<int> munmap(std::weak_ptr<process::Process> process, memory::vaddr_t addr, size_t length) {
    if (memory::pageAlign(addr) != addr || memory::pageAlign(length) != length)
        throw std::invalid_argument("Invalid page or length alignment");

    std::shared_ptr<process::Process>(process)->maps.erase(addr, length / PAGE_SIZE);
    return async::make_ready_future(0);
}

static std::map<memory::vaddr_t, std::pair<memory::Page, bool>> _sharedPages;
async::future<int> sos_share_vm(std::weak_ptr<process::Process> process, memory::vaddr_t addr, size_t length, bool isWritable) {
    static bool _mutex = false;

    if (memory::pageAlign(addr) != addr || memory::pageAlign(length) != length)
        throw std::invalid_argument("Invalid page or length alignment");

    if (_mutex)
        throw std::system_error(ERESTART, std::system_category());
    _mutex = true;

    try {
        // this is relatively dodgy, and could do something dangerous (to the thread that started it),
        // but the syscall is pretty badly designed, and this is the same way linux does it if you call mmap
        std::shared_ptr<process::Process>(process)->maps.erase(addr, length / PAGE_SIZE);

        async::promise<void> promise;
        promise.set_value();
        auto future = promise.get_future();

        memory::vaddr_t end = addr + length;
        for (; addr < end; ) {
            auto pageStart = _sharedPages.lower_bound(addr);
            memory::vaddr_t pageStartAddress = pageStart == _sharedPages.end() ? end : pageStart->first;

            if (pageStartAddress != addr) {
                // Pages don't exist yet - map in as writable
                future = future.then([=](async::future<void> result) {
                    result.get();
                    return syscall::mmap2(
                        process,
                        addr,
                        pageStartAddress - addr,
                        PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_LOCKED | MAP_FIXED,
                        0, 0
                    );
                }).unwrap().then([=](auto result) {
                    assert(static_cast<memory::vaddr_t>(result.get()) == addr);

                    std::shared_ptr<process::Process> _process(process);
                    for (memory::vaddr_t start = addr; start < pageStartAddress; start += PAGE_SIZE) {
                        _sharedPages[start] = std::make_pair(
                            _process->pageDirectory.lookup(start)->getPage().copy(),
                            isWritable
                        );
                    }
                });

                addr = pageStartAddress;
            } else {
                // Pages already exist - find the end of the range
                auto pageEnd = pageStart;
                memory::vaddr_t pageEndAddress;
                while (true) {
                    auto curPage = pageEnd;
                    pageEndAddress = curPage->first;

                    ++pageEnd;
                    if (pageEnd == _sharedPages.end() ||
                        curPage->first + PAGE_SIZE != pageEnd->first ||
                        curPage->second.second != pageEnd->second.second) {
                        break;
                    }
                }
                pageEndAddress += PAGE_SIZE;

                // Map them in with their current writability
                future = future.then([=](async::future<void> result) {
                    result.get();
                    return syscall::mmap2(
                        process,
                        pageStartAddress,
                        pageEndAddress - pageStartAddress,
                        PROT_READ | (pageStart->second.second ? PROT_WRITE : 0),
                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,
                        0, 0
                    );
                }).unwrap().then([=](auto result) {
                    assert(static_cast<memory::vaddr_t>(result.get()) == pageStartAddress);

                    std::vector<async::future<const memory::MappedPage&>> futures;
                    std::shared_ptr<process::Process> _process(process);
                    for (auto page = pageStart; page != pageEnd; ++page) {
                        futures.push_back(_process->pageDirectory.map(
                            page->second.first.copy(),
                            page->first,
                            memory::Attributes{.read = true, .write = page->second.second}
                        ));
                        page->second.second &= isWritable;
                    }

                    return async::when_all(futures.begin(), futures.end())
                        .then([](auto futures) {
                            for (auto& future : futures.get())
                                future.get();
                        });
                });

                addr = pageEndAddress;
            }
        }

        return future.then([](async::future<void> result) {
            _mutex = false;
            result.get();
            return 0;
        });
    } catch (...) {
        _mutex = false;
        throw;
    }
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

        // Incrementally expand the heap, so cap memory can be allocated properly
        // as we fault in pages. Allocating too much at once will make cspace
        // use more memory than available in the init area and end up calling
        // itself (via mmap faulting in pages), resulting in an assertion failure
        constexpr size_t targetSize = memory::SOS_BRK_END - memory::SOS_BRK_START;
        for (size_t size = 2 * memory::SOS_INIT_AREA_SIZE; size < targetSize; size *= 2) {
            assert(mmap(
                brkEnd,
                size - (brkEnd - brkArea),
                PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,
                0, 0
            ) == brkEnd);

            brkEnd = &brkArea[size];
        }
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

        int _result = result.get();
        --_recursionDepth;
        return _result;
    } catch (...) {
        --_recursionDepth;
        return -syscall::exceptionToErrno(std::current_exception());
    }
}

FORWARD_SYSCALL(munmap, 2);
