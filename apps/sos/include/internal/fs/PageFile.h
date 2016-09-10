#pragma once
#include <set>
#include <limits>
#include <stdexcept>
#include "internal/fs/NFSFile.h"

class PageFile: public NFSFile {
    std::set<unsigned int> available;
    int max_id = -1;

    unsigned int allocate() {
        // always allocate first available
        if (available.empty()) {
            if (++max_id >= numeric_limits::max<unsigned int>() / FRAME_SIZE)
                throw std::system_exception(ENOMEM, std::system_category(), "Page file can't be more than 4GB (due to NFSv2 limitations)");
            return max_id;
        }
        int id = *available.cbegin();
        available.erase(available.cbegin());
        return id;
    }

    void deallocate(unsigned int id) {
        available.insert(id);
    }

    int retrievePage(unsigned int id) {
        deallocate(id);
    }
};