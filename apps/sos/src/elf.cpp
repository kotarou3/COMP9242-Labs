#include <stdexcept>
#include <stdint.h>

extern "C" {
    #include <elf/elf.h>
    #include <sel4/types.h>
}

#include "internal/elf.h"
#include "internal/fs/File.h"
#include "internal/memory/Mappings.h"
#include "internal/memory/PageDirectory.h"
#include "internal/memory/UserMemory.h"

namespace elf {

async::future<memory::vaddr_t> load(std::shared_ptr<process::Process> process, const std::string& pathname) {
    return fs::rootFileSystem->open(pathname, fs::FileSystem::OpenFlags{.read = true}).then([=] (auto file) {
        std::shared_ptr<fs::File> _file = std::move(file.get());
        auto elfHeader = std::make_shared<std::array<uint8_t, PAGE_SIZE>>();

        return _file->read(
            {fs::IoVector{
                .buffer = memory::UserMemory(
                    process::getSosProcess(),
                    reinterpret_cast<memory::vaddr_t>(elfHeader.get())
                ),
                .length = PAGE_SIZE
            }},
            0
        ).then([=] (auto read) {
            read.get();

            uint8_t* header = elfHeader->data();
            if (elf32_checkFile((Elf32_Header*)header))
                throw std::invalid_argument("Invalid ELF file");

            size_t headers = elf_getNumProgramHeaders(header);
            for (size_t h = 0; h < headers; ++h) {
                if (elf_getProgramHeaderType(header, h) != PT_LOAD)
                    continue;

                size_t fileOffset = elf_getProgramHeaderOffset(header, h);
                memory::vaddr_t to = elf_getProgramHeaderVaddr(header, h);
                size_t fileSize = elf_getProgramHeaderFileSize(header, h);
                size_t memorySize = elf_getProgramHeaderMemorySize(header, h);
                int flags = elf_getProgramHeaderFlags(header, h);

                if (fileSize > memorySize)
                    throw std::invalid_argument("Segment file size larger than memory size");

                size_t startPadding = memory::pageOffset(to);
                memory::vaddr_t start = to - startPadding;
                size_t pages = memory::numPages(memorySize + startPadding);

                // create a mapping that points to a file, rather than a normal mapping
                // implicit lazy pagefaulting, yay!
                process->maps.insert(
                    start, pages,
                    memory::Attributes{
                        .read = flags & PF_R,
                        .write = flags & PF_W,
                        .execute = flags & PF_X
                    },
                    memory::Mapping::Flags{.shared = false, .fixed = true},
                    _file, fileOffset, fileSize, startPadding
                ).release();
            }

            return static_cast<memory::vaddr_t>(elf_getEntryPoint(header));
        });
    });
}

}
