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
        boost::future<struct stat> _statLoop(const std::string& pathname, std::vector<std::unique_ptr<FileSystem>>::iterator fs);
        boost::future<std::shared_ptr<File>> _openLoop(const std::string& pathname, OpenFlags flags, std::vector<std::unique_ptr<FileSystem>>::iterator fs);

        std::vector<std::unique_ptr<FileSystem>> _filesystems;

        friend class FlatDirectory;
};

class FlatDirectory : public Directory {
    public:
        FlatDirectory(FlatFileSystem& fs);
        virtual ~FlatDirectory() override = default;

        virtual boost::future<ssize_t> getdents(memory::UserMemory dirp, size_t length);

    private:
        FlatFileSystem& _fs;

        size_t _currentFsIndex;
        std::shared_ptr<Directory> _currentFsDirectory;
};

}
