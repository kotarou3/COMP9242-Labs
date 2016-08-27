#include <stdexcept>

#include "internal/fs/FlatFileSystem.h"

namespace fs {

boost::future<std::shared_ptr<File>> FlatFileSystem::open(const std::string& pathname) {
    for (auto& fs : _filesystems) try {
        return fs->open(pathname);
    } catch (...) {
        // Do nothing
        // TODO: Actually check that the error was because the pathname doesn't exist
    }

    throw std::invalid_argument("File does not exist");
}

void FlatFileSystem::mount(std::unique_ptr<FileSystem> fs) {
    _filesystems.emplace_back(std::move(fs));
}

}
