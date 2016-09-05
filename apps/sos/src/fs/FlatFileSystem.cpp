#include <system_error>

#include "internal/fs/FlatFileSystem.h"

namespace fs {

////////////////////
// FlatFileSystem //
////////////////////

boost::future<struct stat> FlatFileSystem::stat(const std::string& pathname) {
    return _statLoop(pathname, _filesystems.begin());
}

boost::future<std::shared_ptr<File>> FlatFileSystem::open(const std::string& pathname, OpenFlags flags) {
    if (pathname == ".")
        return boost::make_ready_future(std::shared_ptr<File>(new FlatDirectory(*this)));

    OpenFlags _flags = flags;
    if (flags.createOnMissing) {
        // First scan through without creation (so we don't accidentally create
        // a file when it exists in a later filesystem)
        _flags.createOnMissing = false;
    }

    boost::future<std::shared_ptr<File>> future;
    try {
        future = _openLoop(pathname, _flags, _filesystems.begin());
    } catch (const std::system_error& e) {
        if (!flags.createOnMissing)
            throw;

        future = boost::make_exceptional_future<std::shared_ptr<File>>(e);
    }

    if (!flags.createOnMissing)
        return future;

    return future.then(asyncExecutor, [this, pathname, flags](auto file) {
        try {
            return boost::make_ready_future(file.get());
        } catch (const std::system_error& e) {
            if (e.code() != std::error_code(ENOENT, std::system_category()))
                throw;
            return this->_openLoop(pathname, flags, _filesystems.begin());
        }
    });
}

void FlatFileSystem::mount(std::unique_ptr<FileSystem> fs) {
    _filesystems.emplace_back(std::move(fs));
}

boost::future<struct stat> FlatFileSystem::_statLoop(const std::string& pathname, std::vector<std::unique_ptr<FileSystem>>::iterator fs) {
    if (fs == _filesystems.end())
        throw std::system_error(ENOENT, std::system_category(), "File does not exist");

    try {
        return (*fs)->stat(pathname).then(asyncExecutor, [=](auto stat) mutable {
            try {
                return boost::make_ready_future(stat.get());
            } catch (const std::system_error& e) {
                if (e.code() != std::error_code(ENOENT, std::system_category()))
                    throw;
                return this->_statLoop(pathname, ++fs);
            }
        });
    } catch (const std::system_error& e) {
        if (e.code() != std::error_code(ENOENT, std::system_category()))
            throw;
        return _statLoop(pathname, ++fs);
    }
}

boost::future<std::shared_ptr<File>> FlatFileSystem::_openLoop(const std::string& pathname, OpenFlags flags, std::vector<std::unique_ptr<FileSystem>>::iterator fs) {
    if (fs == _filesystems.end()) {
        throw std::system_error(
            ENOENT, std::system_category(),
            flags.createOnMissing ? "Filesystems failed to create file" : "File does not exist"
        );
    }

    try {
        return (*fs)->open(pathname, flags).then(asyncExecutor, [=](auto file) mutable {
            try {
                return boost::make_ready_future(file.get());
            } catch (const std::system_error& e) {
                if (e.code() != std::error_code(ENOENT, std::system_category()))
                    throw;
                return this->_openLoop(pathname, flags, ++fs);
            }
        });
    } catch (const std::system_error& e) {
        if (e.code() != std::error_code(ENOENT, std::system_category()))
            throw;
        return _openLoop(pathname, flags, ++fs);
    }
}

///////////////////
// FlatDirectory //
///////////////////

FlatDirectory::FlatDirectory(FlatFileSystem& fs):
    _fs(fs),
    _currentFsIndex(0)
{}

boost::future<ssize_t> FlatDirectory::getdents(memory::UserMemory dirp, size_t length) {
    if (_currentFsIndex >= _fs._filesystems.size())
        return boost::make_ready_future(0);

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

            return boost::make_ready_future(_bytesWritten);
        }).unwrap();
}

}
