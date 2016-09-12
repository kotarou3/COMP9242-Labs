#pragma once
#include <set>
#include <limits>
#include <stdexcept>
#include "internal/fs/NFSFile.h"

namespace memory {

class PageFile {
    PageFile():
        vaddr{process::getSosProcess().maps.insertPermanent(
                1, Attributes{.read=true, .write=true}, Mapping::Flags{.shared = false}
        )},
        buffer{UserMemory(process::getSosProcess(), vaddr)}
    {
        fs::rootFileSystem->open("pagefile",
            fs::FileSystem::OpenFlags{.read=true, .write=true, .createOnMissing=true, .truncate=true, .mode=0666}
        ).then(fs::asyncExecutor, [this] (auto value) {
            this->store = value.get();
        });
    }

    unsigned int allocate() {
        // always prefer things in available, because overwriting them doesn't use up an extra disk block in the pagefile
        if (available.empty()) {
            if (++max_id >= std::numeric_limits<unsigned int>::max() / PAGE_SIZE)
                throw std::system_error(ENOMEM, std::system_category(),
                                        "Page file can't be more than 4GB (due to NFSv2 limitations)");
            return max_id;
        }
        int id = *available.cbegin();
        available.erase(available.cbegin());
        return id;
    }

    void deallocate(unsigned int id) {
        available.insert(id);
    }

    std::set<unsigned int> available;
    unsigned int max_id = -1;
    std::shared_ptr<fs::File> store;
    vaddr_t vaddr;
    memory::UserMemory buffer;
public:
    static PageFile& get() {
        static auto f = PageFile{};
        return f;
    }

    unsigned int page(Page& page) {
        unsigned int id = allocate();
        assert(seL4_ARM_Page_Map(
                page.getCap(), process::getSosProcess().pageDirectory.getCap(),
                buffer._address, static_cast<seL4_CapRights>(seL4_CanRead | seL4_CanWrite), seL4_ARM_ExecuteNever
        ) == seL4_NoError);
        store->write(std::vector<fs::IoVector>{fs::IoVector{buffer, PAGE_SIZE}}, id * PAGE_SIZE);
        assert(seL4_ARM_Page_Unmap(page.getCap()) == seL4_NoError);
        return id;
    }
};

}