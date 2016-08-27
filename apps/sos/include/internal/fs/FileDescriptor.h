#pragma once

#include <map>
#include <memory>

namespace fs {

using FileDescriptor = int;
constexpr const size_t MAX_FILE_DESCRIPTORS = 1000;

class File;

class OpenFile {
    public:
        struct Flags {
            bool read:1, write:1;
        };

        OpenFile(std::shared_ptr<File> file, Flags flags);

        std::shared_ptr<File> get(Flags flags) const;

    private:
        std::shared_ptr<File> _file;
        Flags _flags;
};

class FDTable {
    public:
        FileDescriptor insert(std::shared_ptr<OpenFile> file);
        bool erase(FileDescriptor fd) noexcept;

        std::shared_ptr<OpenFile> get(FileDescriptor fd) const;
        std::shared_ptr<File> get(FileDescriptor fd, OpenFile::Flags flags) const;

    private:
        std::map<FileDescriptor, std::shared_ptr<OpenFile>> _table;
};

}
