
#include <cassert>
#include <cerrno>
#include <cstdarg>
#include <cstdint>
#include <sys/mman.h>
#include <sys/stat.h>

#include "internal/memory/UserMemory.h"
#include "internal/process/Thread.h"
#include "internal/fs/FileDescriptor.h"

extern "C" {
    #include <sos.h>
}

namespace syscall {

int open(process::Process& process, memory::vaddr_t fnameaddr, int mode) noexcept {
    // TODO: check user has permission to read from the filename vaddr
    try {
        std::string fileName = memory::UserMemory(process, fnameaddr).readString(1000, false);
        auto fdmode = fs::Mode{.read=mode & S_IRUSR, .write=mode & S_IWUSR, .execute=mode & S_IXUSR};
        try {
            return process.fdTable.open(fileName, fdmode);
        } catch (std::runtime_error& e) {
            assert(!"Failed to open file. This is milestone 5's problem");
        }
    } catch (std::runtime_error&) {
        // string was invalid. Not sure what error message to return cause I don't have internet
    }
}

}
