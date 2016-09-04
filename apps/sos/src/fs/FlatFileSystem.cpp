#include <system_error>

#include "internal/fs/FlatFileSystem.h"
#include "internal/syscall/helpers.h"

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

boost::future<std::unique_ptr<nfs::fattr_t>> FlatFileSystem::stat(const std::string& pathname) {
    boost::future<std::unique_ptr<nfs::fattr_t>> result;
    for (auto& fs : _filesystems) {
        result = fs->stat(pathname);
        if (!result.has_exception()) {
            return result;
        }
    }
    return result;
}

void FlatFileSystem::mount(std::unique_ptr<FileSystem> fs) {
    _filesystems.emplace_back(std::move(fs));
}

}
