#include <stdexcept>
#include <stdint.h>

#include "internal/elf.h"
#include "internal/memory/Mappings.h"
#include "internal/memory/PageDirectory.h"
#include "internal/memory/UserMemory.h"

extern "C" {
    #include <elf/elf.h>
    #include <sel4/types.h>
}

namespace elf {

async::future<memory::vaddr_t> load(process::Process& process, uint8_t* file) {
    if (elf_checkFile(file))
        throw std::invalid_argument("Invalid ELF file");

    std::vector<std::shared_ptr<memory::ScopedMapping>> maps;
    std::vector<async::future<void>> writes;

    size_t headers = elf_getNumProgramHeaders(file);
    for (size_t h = 0; h < headers; ++h) {
        if (elf_getProgramHeaderType(file, h) != PT_LOAD)
            continue;

        uint8_t* from = file + elf_getProgramHeaderOffset(file, h);
        memory::vaddr_t to = elf_getProgramHeaderVaddr(file, h);
        size_t fileSize = elf_getProgramHeaderFileSize(file, h);
        size_t memorySize = elf_getProgramHeaderMemorySize(file, h);
        int flags = elf_getProgramHeaderFlags(file, h);

        if (fileSize > memorySize)
            throw std::invalid_argument("Segment file size larger than memory size");

        size_t startPadding = memory::pageOffset(to);
        memory::vaddr_t start = to - startPadding;
        size_t pages = memory::numPages(memorySize + startPadding);

        auto map = std::make_shared<memory::ScopedMapping>(process.maps.insert(
            start, pages,
            memory::Attributes{
                .read = flags & PF_R,
                .write = flags & PF_W,
                .execute = flags & PF_X
            },
            memory::Mapping::Flags{.shared = false, .fixed = true}
        ));

        if (fileSize > 0) {
            writes.push_back(
                memory::UserMemory(process, to).write(from, from + fileSize, true)
                    .then([&process, map, startPadding, fileSize](async::future<void> result) {
                        result.get();

                        // Unify the data and instruction cache's view of the pages
                        for (size_t b = 0; b < fileSize + startPadding; b += PAGE_SIZE) {
                            assert(seL4_ARM_Page_Unify_Instruction(
                                process.pageDirectory.lookup(map->getStart() + b)->getPage().getCap(),
                                0, PAGE_SIZE
                            ) == seL4_NoError);
                        }
                    })
            );
        }

        maps.push_back(std::move(map));
    }

    return async::when_all(writes.begin(), writes.end()).then([maps, file](auto results) {
        for (auto& result : results.get())
            result.get();

        for (auto& map : maps)
            map->release();

        return static_cast<memory::vaddr_t>(elf_getEntryPoint(file));
    });
}

}
