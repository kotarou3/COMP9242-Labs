#pragma once

#include "internal/fs/File.h"

struct serial;

namespace fs {

class DeviceFileSystem;

class ConsoleDevice : public File {
    public:
        static void init(DeviceFileSystem& fs);

        virtual boost::future<ssize_t> read(const std::vector<IoVector>& iov, off64_t offset) override;
        virtual boost::future<ssize_t> write(const std::vector<IoVector>& iov, off64_t offset) override;

    private:
        ConsoleDevice() = default;
};

}
