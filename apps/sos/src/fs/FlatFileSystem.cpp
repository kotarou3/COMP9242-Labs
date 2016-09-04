#include <system_error>

#include "internal/fs/FlatFileSystem.h"

namespace fs {

boost::future<struct stat> FlatFileSystem::stat(const std::string& pathname) {
    for (auto& fs : _filesystems) try {
        return fs->stat(pathname);
    } catch (const std::system_error& e) {
        if (e.code() != std::error_code(ENOENT, std::system_category()))
            throw;
    }

    throw std::system_error(ENOENT, std::system_category(), "File does not exist");
}

boost::future<std::shared_ptr<File>> FlatFileSystem::open(const std::string& pathname, OpenFlags flags) {
    // XXX: Loop is broken due to async - needs fixing

    OpenFlags _flags = flags;
    _flags.createOnMissing = false;

    for (auto& fs : _filesystems) try {
        return fs->open(pathname, _flags);
    } catch (const std::system_error& e) {
        if (e.code() != std::error_code(ENOENT, std::system_category()))
            throw;
    }

    for (auto& fs : _filesystems) try {
        return fs->open(pathname, flags);
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
