#include <vector>

#include "internal/memory/PageDirectory.h"
#include "internal/process/Thread.h"

namespace memory {

class UserMemory {
    public:
        UserMemory(process::Process& process, vaddr_t address);

        std::vector<uint8_t> read(size_t bytes, bool bypassAttributes = false);
        void write(uint8_t* from, size_t bytes, bool bypassAttributes = false);

    private:
        ScopedMapping _mapIn(size_t bytes, Attributes attributes, bool bypassAttributes);

        process::Process& _process;
        vaddr_t _address;
};

}
