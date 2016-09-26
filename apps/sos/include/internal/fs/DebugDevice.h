#pragma once

#include "internal/fs/File.h"

struct serial;

namespace fs {

class DebugDevice : public File {
    public:
        virtual async::future<int> ioctl(size_t request, memory::UserMemory argp) override;

    protected:
        virtual async::future<ssize_t> _writeOne(const IoVector& iov, off64_t offset) override;
};

}
