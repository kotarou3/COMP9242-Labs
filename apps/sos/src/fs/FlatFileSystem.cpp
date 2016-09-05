#include <system_error>

#include "internal/fs/FlatFileSystem.h"

namespace fs {

////////////////////
// FlatFileSystem //
////////////////////

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
    if (pathname == ".") {
        boost::promise<std::shared_ptr<File>> promise;
        promise.set_value(std::make_shared<FlatDirectory>(*this));
        return promise.get_future();
    }

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

///////////////////
// FlatDirectory //
///////////////////

FlatDirectory::FlatDirectory(FlatFileSystem& fs):
    _fs(fs),
    _currentFsIndex(0)
{}

boost::future<ssize_t> FlatDirectory::getdents(memory::UserMemory dirp, size_t length) {
    if (_currentFsIndex >= _fs._filesystems.size()) {
        boost::promise<ssize_t> promise;
        promise.set_value(0);
        return promise.get_future();
    }

    if (!_currentFsDirectory) {
        return _fs._filesystems[_currentFsIndex]->open(".", FileSystem::OpenFlags{})
            .then(asyncExecutor, [=](auto file) mutable {
                this->_currentFsDirectory = std::dynamic_pointer_cast<Directory>(file.get());
                if (!this->_currentFsDirectory)
                    // Filesystem doesn't support directory listings - skip it
                    ++this->_currentFsIndex;

                return this->getdents(dirp, length);
            }).unwrap();
    }

    return _currentFsDirectory->getdents(dirp, length)
        .then(asyncExecutor, [=](auto bytesWritten) mutable {
            auto _bytesWritten = bytesWritten.get();
            if (_bytesWritten == 0) {
                // Finished directory listing - go to next filesystem
                this->_currentFsDirectory.reset();
                ++this->_currentFsIndex;
                return this->getdents(dirp, length);
            }

            boost::promise<ssize_t> promise;
            promise.set_value(_bytesWritten);
            return promise.get_future();
        }).unwrap();
}

}
