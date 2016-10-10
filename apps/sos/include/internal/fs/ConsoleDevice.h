#pragma once

#include <string>

#include "internal/fs/File.h"

struct serial;

namespace fs {

class DeviceFileSystem;

class ConsoleDevice : public File {
    public:
        static void mountOn(DeviceFileSystem& fs, const std::string& name);

        virtual async::future<int> ioctl(size_t request, memory::UserMemory argp) override;

    protected:
        virtual async::future<ssize_t> _readOne(const IoVector& iov, off64_t offset, bool bypassAttributes) override;
        virtual async::future<ssize_t> _writeOne(const IoVector& iov, off64_t offset) override;

    private:
        ConsoleDevice() = default;
};

}
