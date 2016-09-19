#pragma once
#include <set>
#include <limits>
#include <stdexcept>
#include "internal/fs/NFSFile.h"

namespace memory {

class Swap {
        Swap():
            vaddr{process::getSosProcess().maps.insertPermanent(
                swap_pages, Attributes{.read=true, .write=true}, Mapping::Flags{.shared = false}
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
                if (++max_id >= std::numeric_limits<unsigned int>::max() / swap_size)
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
        memory::UserMemory buffer; // needs to be usermemory because iovec has usermemory
    public:
        constexpr static auto swap_pages = 10U;
        constexpr static auto swap_size = swap_pages * PAGE_SIZE;

        static Swap& get() {
            static auto f = Swap{};
            return f;
        }

        boost::future<unsigned int> swapout(std::vector<Page*> pages) {
            unsigned int id = allocate();
            for (auto i = 0U; i < swap_pages; ++i)
                process::getSosProcess().pageDirectory.map(pages[i]->copy(), vaddr + PAGE_SIZE * i, Attributes{.read=true, .write=true});

            return store->write(std::vector<fs::IoVector>{fs::IoVector{buffer, swap_size}}, id * swap_size).then(
                fs::asyncExecutor, [id, this] (auto size) {
                    if (size.get() != swap_size)
                        throw std::system_error(ENOMEM, std::system_category(), "Failed to swap out page");
                    for (auto i = 0U; i < swap_pages; ++i)
                        process::getSosProcess().pageDirectory.unmap(vaddr + PAGE_SIZE * i);
                    return boost::make_ready_future(id);
                }
            );
        }
};

}