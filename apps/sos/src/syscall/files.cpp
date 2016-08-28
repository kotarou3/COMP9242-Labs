
#include <cassert>
#include <cerrno>
#include <cstdarg>
#include <cstdint>
#include <sys/mman.h>

#include "internal/memory/UserMemory.h"
#include "internal/process/Thread.h"
#include "internal/fs/FileDescriptor.h"

extern "C" {
    #include <sos.h>
}

namespace syscall {

int open(process::Process& process, memory::vaddr_t fname, int mode) noexcept {
    // TODO: check user has permission to read from the filename vaddr
    try {
        std::string fname = memory::UserMemory(process, fname).readString();
    } catch (std::runtime_error&) {
        // string was invalid. Not sure what error message to return cause I don't have internet
    }
    auto fdmode = fs::Mode{.read=mode & FM_READ, .write=mode & FM_WRITE, .execute=mode & FM_EXEC};
    try {
        return process.fdTable.open(fname, fdmode);
    } catch (std::runtime_exception(e)) {
        assert(!"Failed to open file. This is milestone 5's problem");
    }
}

}
