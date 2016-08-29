#include <stdexcept>

#include "internal/fs/FileDescriptor.h"

namespace fs {

//////////////
// OpenFile //
//////////////

OpenFile::OpenFile(std::shared_ptr<File> file, OpenFile::Flags flags):
    _file(std::move(file)),
    _flags(flags)
{}

std::shared_ptr<File> OpenFile::get(OpenFile::Flags flags) const {
    if (flags.read && !_flags.read)
        throw std::runtime_error("File not open for reading");
    if (flags.write && !_flags.write)
        throw std::runtime_error("File not open for writing");

    return _file;
}

/////////////
// FDTable //
/////////////

FileDescriptor FDTable::insert(std::shared_ptr<OpenFile> file) {
    if (_table.size() >= MAX_FILE_DESCRIPTORS)
        throw std::runtime_error("Too many file descriptors being used");

    auto entry = _table.cbegin();
    if (entry == _table.cend() || entry->first != 0) {
        _table.insert(std::make_pair(0, std::move(file)));
        return 0;
    }

    FileDescriptor nextFd = entry->first + 1;
    for (++entry; entry != _table.cend(); ++entry) {
        if (entry->first != nextFd) {
            _table.insert(std::make_pair(nextFd, std::move(file)));
            return nextFd;
        }
        nextFd = entry->first + 1;
    }

    _table.insert(std::make_pair(nextFd, std::move(file)));
    return nextFd;
}

bool FDTable::erase(FileDescriptor fd) noexcept {
    return _table.erase(fd);
}

std::shared_ptr<OpenFile> FDTable::get(FileDescriptor fd) const {
    return _table.at(fd);
}

std::shared_ptr<File> FDTable::get(FileDescriptor fd, OpenFile::Flags flags) const {
    return get(fd)->get(flags);
}

}
