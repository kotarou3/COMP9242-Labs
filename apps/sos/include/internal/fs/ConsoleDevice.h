#pragma once

#include <string>

#include "internal/fs/File.h"

struct serial;

namespace fs {

class DeviceFileSystem;

class ConsoleDevice : public File {
    public:
        static void mountOn(DeviceFileSystem& fs, const std::string& name);

        virtual boost::future<ssize_t> read(const std::vector<IoVector>& iov, off64_t offset) override;
        virtual boost::future<ssize_t> write(const std::vector<IoVector>& iov, off64_t offset) override;

        virtual boost::future<int> ioctl(size_t request, memory::UserMemory argp) override;

    private:
        ConsoleDevice() = default;
};

}
