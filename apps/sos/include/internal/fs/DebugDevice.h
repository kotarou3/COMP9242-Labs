#pragma once

#include "internal/fs/File.h"

struct serial;

namespace fs {

class DebugDevice : public File {
    public:
        virtual boost::future<ssize_t> write(const std::vector<IoVector>& iov, off64_t offset) override;

        virtual boost::future<int> ioctl(size_t request, memory::UserMemory argp) override;
};

}
