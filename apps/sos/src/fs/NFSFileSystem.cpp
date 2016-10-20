#include <stdexcept>
#include <system_error>

extern "C" {
    #include <autoconf.h>
}

#include "internal/fs/NFSFile.h"
#include "internal/fs/NFSFileSystem.h"

namespace fs {

NFSFileSystem::NFSFileSystem(const std::string& serverIp, const std::string& nfsDir) {
    ip_addr ip;
    if (!ipaddr_aton(CONFIG_SOS_GATEWAY, &ip))
        throw std::invalid_argument(serverIp + " is not a valid IPv4 address");
    nfs::init(ip);

    _handle = nfs::mount(nfsDir);
    _timeoutTimer = timer::setTimer(std::chrono::milliseconds(100), nfs::timeout, true);
}

NFSFileSystem::~NFSFileSystem() {
    timer::clearTimer(_timeoutTimer);
}

async::future<struct stat> NFSFileSystem::stat(const std::string& pathname) {
    return nfs::lookup(_handle, pathname)
        .then([](auto result) {
            auto _result = result.get().second;

            struct stat result2 = {0};
            result2.st_dev = _result->fsid;
            result2.st_ino = _result->fileid;
            result2.st_mode = _result->mode;
            result2.st_nlink = _result->nlink;
            result2.st_uid = _result->uid;
            result2.st_gid = _result->gid;
            result2.st_rdev = _result->rdev;
            result2.st_size = _result->size;
            result2.st_blksize = _result->block_size;
            result2.st_blocks = _result->blocks;
            result2.st_atim = {
                .tv_sec = static_cast<time_t>(_result->atime.seconds),
                .tv_nsec = static_cast<long>(_result->atime.useconds * 1000)
            };
            result2.st_mtim = {
                .tv_sec = static_cast<time_t>(_result->mtime.seconds),
                .tv_nsec = static_cast<long>(_result->mtime.useconds * 1000)
            };
            result2.st_ctim = {
                .tv_sec = static_cast<time_t>(_result->ctime.seconds),
                .tv_nsec = static_cast<long>(_result->ctime.useconds * 1000)
            };
            return result2;
        });
}

async::future<std::shared_ptr<File>> NFSFileSystem::open(const std::string& pathname, OpenFlags flags) {
    return nfs::lookup(_handle, pathname)
        .then([=](auto result) {
            try {
                if (result.has_exception())
                    result.get();
                return result;
            } catch (const std::system_error& e) {
                if (e.code() != std::error_code(ENOENT, std::system_category()))
                    throw;
                if (!flags.createOnMissing)
                    throw;

                return nfs::create(this->_handle, pathname, nfs::sattr_t{
                    .mode = flags.mode,
                    .uid = -1U,
                    .gid = -1U,
                    .size = 0,
                    .atime = {.seconds = -1U, .useconds = -1U},
                    .mtime = {.seconds = -1U, .useconds = -1U}
                });
            }
        }).unwrap().then([=](auto result) {
            auto _result = result.get();

            // XXX: Would check permissions here, but we have no idea what user
            // we end up using on the nfs (so files that look like they're
            // read/writable might not be, and vice versa). So we allow the user
            // to open the file regardless, and let the NFS do the actual
            // permission checking on the actual read/write.

            if (S_ISDIR(_result.second->mode))
                return std::shared_ptr<File>(new NFSDirectory(*_result.first));
            else
                return std::shared_ptr<File>(new NFSFile(*_result.first, !flags.direct));
        });
}

}
