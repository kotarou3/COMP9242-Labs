#pragma once

#include "internal/fs/File.h"

namespace fs {

class FlatFileSystem : public FileSystem {
    public:
        virtual ~FlatFileSystem() override = default;

        // pathname will be checked through the mounted filesystems in the order of mounting
        virtual boost::future<struct stat> stat(const std::string& pathname) override;
        virtual boost::future<std::shared_ptr<File>> open(const std::string& pathname, OpenFlags flags) override;

        void mount(std::unique_ptr<FileSystem> fs);

    private:
        std::vector<std::unique_ptr<FileSystem>> _filesystems;
};

}
