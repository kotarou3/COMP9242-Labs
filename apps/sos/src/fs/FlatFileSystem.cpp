#include <system_error>

#include "internal/fs/FlatFileSystem.h"

namespace fs {

boost::future<std::shared_ptr<File>> FlatFileSystem::open(const std::string& pathname) {
    for (auto& fs : _filesystems) try {
        return fs->open(pathname);
    } catch (const std::system_error& e) {
        if (e.code() != std::error_code(ENOENT, std::system_category()))
            throw;
    }

    throw std::system_error(ENOENT, std::system_category(), "File does not exist");
}

void FlatFileSystem::mount(std::unique_ptr<FileSystem> fs) {
    _filesystems.emplace_back(std::move(fs));
}

}
