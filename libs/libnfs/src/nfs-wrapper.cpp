#include <algorithm>
#include <limits>
#include <stdexcept>
#include <system_error>
#include <unordered_map>

#include <assert.h>

extern "C" {
    #include <lwip/pbuf.h>
}

#include "nfs.h"

namespace nfs {

namespace {
    template <typename T>
    [[noreturn]] void _throwRpcError(rpc_stat_t e, T description) {
        int err = ECOMM;
        switch (e) {
            case RPC_OK:
                // ???
                assert(false);

            #define ADD_STANDARD_ERR(e) case RPCERR_##e: err = E##e; break
            ADD_STANDARD_ERR(NOMEM);
            ADD_STANDARD_ERR(COMM);
            #undef ADD_STANDARD_ERR

            case RPCERR_NOBUF:
                err = ENOBUFS;
                break;

            case RPCERR_NOSUP:
                err = ENOSYS;
                break;
        }

        throw std::system_error(err, std::system_category(), description);
    }

    template <typename T>
    [[noreturn]] void _throwNfsError(nfs_stat_t e, T description) {
        int err = ECOMM;
        switch (e) {
            case NFS_OK:
                // ???
                assert(false);

            #define ADD_STANDARD_ERR(e) case NFSERR_##e: err = E##e; break
            ADD_STANDARD_ERR(PERM);
            ADD_STANDARD_ERR(NOENT);
            ADD_STANDARD_ERR(IO);
            ADD_STANDARD_ERR(NXIO);
            ADD_STANDARD_ERR(ACCES);
            ADD_STANDARD_ERR(EXIST);
            ADD_STANDARD_ERR(NODEV);
            ADD_STANDARD_ERR(NOTDIR);
            ADD_STANDARD_ERR(ISDIR);
            ADD_STANDARD_ERR(FBIG);
            ADD_STANDARD_ERR(NOSPC);
            ADD_STANDARD_ERR(ROFS);
            ADD_STANDARD_ERR(NAMETOOLONG);
            ADD_STANDARD_ERR(NOTEMPTY);
            ADD_STANDARD_ERR(DQUOT);
            ADD_STANDARD_ERR(STALE);
            ADD_STANDARD_ERR(COMM);
            #undef ADD_STANDARD_ERR

            case NFSERR_WFLUSH:
                err = EIO;
                break;
        }

        throw std::system_error(err, std::system_category(), description);
    }

    struct ReaddirRequest {
        fhandle_t handle;
        boost::promise<std::vector<std::string>> promise;
        std::vector<std::string> result;
    };
    std::unordered_map<size_t, ReaddirRequest> _readdirRequests;
    size_t _readdirRequestsNextId;
    void _readdirCallback(size_t id, nfs_stat_t e, int length, char** files, nfscookie_t nfsCookie) noexcept try {
        if (e != NFS_OK)
            _throwNfsError(e, "Failed to read a NFS directory");
        if (length < 0)
            throw std::system_error(ECOMM, std::system_category(), "NFS returned a negative length");

        auto& request = _readdirRequests.at(id);
        request.result.insert(request.result.end(), &files[0], &files[length]);

        if (nfsCookie == 0) {
            request.promise.set_value(std::move(request.result));
            _readdirRequests.erase(id);
        } else {
            rpc_stat_t e = nfs_readdir(&request.handle, nfsCookie, _readdirCallback, id);
            if (e != RPC_OK)
                _throwRpcError(e, "Failed to read a NFS directory");
        }
    } catch (...) {
        _readdirRequests.at(id).promise.set_exception(std::current_exception());
        _readdirRequests.erase(id);
    }

    std::unordered_map<size_t, boost::promise<std::pair<const fhandle_t*, fattr_t*>>> _lookupRequests;
    size_t _lookupRequestsNextId;
    void _lookupCallback(size_t id, nfs_stat_t e, fhandle_t* handle, fattr_t* attr) noexcept try {
        if (e != NFS_OK)
            _throwNfsError(e, "Failed to lookup a NFS file");

        _lookupRequests.at(id).set_value(std::make_pair(handle, attr));
        _lookupRequests.erase(id);
    } catch (...) {
        _lookupRequests.at(id).set_exception(std::current_exception());
        _lookupRequests.erase(id);
    }

    std::unordered_map<size_t, boost::promise<std::pair<const fhandle_t*, fattr_t*>>> _createRequests;
    size_t _createRequestsNextId;
    void _createCallback(size_t id, nfs_stat_t e, fhandle_t* handle, fattr_t* attr) noexcept try {
        if (e != NFS_OK)
            _throwNfsError(e, "Failed to create a NFS file");

        _createRequests.at(id).set_value(std::make_pair(handle, attr));
        _createRequests.erase(id);
    } catch (...) {
        _createRequests.at(id).set_exception(std::current_exception());
        _createRequests.erase(id);
    }

    std::unordered_map<size_t, boost::promise<void>> _removeRequests;
    size_t _removeRequestsNextId;
    void _removeCallback(size_t id, nfs_stat_t e) noexcept try {
        if (e != NFS_OK)
            _throwNfsError(e, "Failed to remove a NFS file");

        _removeRequests.at(id).set_value();
        _removeRequests.erase(id);
    } catch (...) {
        _removeRequests.at(id).set_exception(std::current_exception());
        _removeRequests.erase(id);
    }

    std::unordered_map<size_t, boost::promise<fattr_t*>> _getattrRequests;
    size_t _getattrRequestsNextId;
    void _getattrCallback(size_t id, nfs_stat_t e, fattr_t* attr) noexcept try {
        if (e != NFS_OK)
            _throwNfsError(e, "Failed to get a NFS file's attributes");

        _getattrRequests.at(id).set_value(attr);
        _getattrRequests.erase(id);
    } catch (...) {
        _getattrRequests.at(id).set_exception(std::current_exception());
        _getattrRequests.erase(id);
    }

    struct ReadRequest {
        fhandle_t handle;
        off_t offset;
        size_t count;
        uint8_t* data;

        size_t readBytes;
        boost::promise<size_t> promise;
    };
    std::unordered_map<size_t, ReadRequest> _readRequests;
    size_t _readRequestsNextId;
    void _readCallback(size_t id, nfs_stat_t e, fattr_t* attr, pbuf* packet, int length, int pos) noexcept try {
        auto& request = _readRequests.at(id);

        if (e != NFS_OK) {
            if (request.readBytes != 0)
                goto resolvePromise;
            _throwNfsError(e, "Failed to read from a NFS file");
        }
        if (length < 0 || pos < 0) {
            if (request.readBytes != 0)
                goto resolvePromise;
            throw std::system_error(ECOMM, std::system_category(), "NFS returned a negative length");
        }

        pbuf_copy_partial(packet, request.data + request.readBytes, length, pos);
        request.readBytes += length;

        if (length > 0 && request.readBytes < request.count) {
            rpc_stat_t e = nfs_read(
                &request.handle,
                request.offset + request.readBytes,
                std::min(request.count - request.readBytes, 6U*1026U),
                _readCallback, id
            );
            if (e == RPC_OK)
                return;
        }

    resolvePromise:
        request.promise.set_value(static_cast<size_t>(request.readBytes));
        _readRequests.erase(id);
    } catch (...) {
        _readRequests.at(id).promise.set_exception(std::current_exception());
        _readRequests.erase(id);
    }

    struct WriteRequest {
        fhandle_t handle;
        off_t offset;
        size_t count;
        const uint8_t* data;

        size_t writtenBytes;
        boost::promise<size_t> promise;
    };
    std::unordered_map<size_t, WriteRequest> _writeRequests;
    size_t _writeRequestsNextId;
    void _writeCallback(size_t id, nfs_stat_t e, fattr_t* /*attr*/, int length) noexcept try {
        auto& request = _writeRequests.at(id);

        if (e != NFS_OK) {
            if (request.writtenBytes != 0)
                goto resolvePromise;
            _throwNfsError(e, "Failed to write to a NFS file");
        }
        if (length < 0) {
            if (request.writtenBytes != 0)
                goto resolvePromise;
            throw std::system_error(ECOMM, std::system_category(), "NFS returned a negative length");
        }

        request.writtenBytes += length;
        if (length > 0 && request.writtenBytes < request.count) {
            rpc_stat_t e = nfs_write(
                &request.handle,
                request.offset + request.writtenBytes,
                request.count - request.writtenBytes,
                request.data + request.writtenBytes,
                _writeCallback, id
            );
            if (e == RPC_OK)
                return;
        }

    resolvePromise:
        request.promise.set_value(static_cast<size_t>(request.writtenBytes));
        _writeRequests.erase(id);
    } catch (...) {
        _writeRequests.at(id).promise.set_exception(std::current_exception());
        _writeRequests.erase(id);
    }
}

void init(const ip_addr& server) {
    rpc_stat_t e = nfs_init(&server);
    if (e != RPC_OK)
        _throwRpcError(e, "Failed to initialise NFS");
}

void timeout() noexcept {
    nfs_timeout();
}

const fhandle_t mount(const std::string& dir) {
    fhandle_t result;
    rpc_stat_t e = nfs_mount(dir.c_str(), &result);
    if (e != RPC_OK)
        _throwRpcError(e, "Failed to mount NFS");

    return result;
}

void print_exports() {
    rpc_stat_t e = nfs_print_exports();
    if (e != RPC_OK)
        _throwRpcError(e, "Failed to print NFS exports");
}

boost::future<std::vector<std::string>> readdir(const fhandle_t& pfh) {
    while (_readdirRequests.count(_readdirRequestsNextId) > 0)
        ++_readdirRequestsNextId;
    size_t id = _readdirRequestsNextId++;
    auto& request = _readdirRequests[id] = {.handle = pfh};
    auto future = request.promise.get_future();

    rpc_stat_t e = nfs_readdir(&request.handle, 0, _readdirCallback, id);
    if (e != RPC_OK) {
        _readdirRequests.erase(id);
        _throwRpcError(e, "Failed to read a NFS directory");
    }

    return future;
}

boost::future<std::pair<const fhandle_t*, fattr_t*>> lookup(const fhandle_t& pfh, const std::string& name) {
    while (_lookupRequests.count(_lookupRequestsNextId) > 0)
        ++_lookupRequestsNextId;
    size_t id = _lookupRequestsNextId++;
    auto& request = _lookupRequests[id];
    auto future = request.get_future();

    rpc_stat_t e = nfs_lookup(&pfh, name.c_str(), _lookupCallback, id);
    if (e != RPC_OK) {
        _lookupRequests.erase(id);
        _throwRpcError(e, "Failed to lookup a NFS file");
    }

    return future;
}

boost::future<std::pair<const fhandle_t*, fattr_t*>> create(const fhandle_t& pfh, const std::string& name, const sattr_t& sattr) {
    while (_createRequests.count(_createRequestsNextId) > 0)
        ++_createRequestsNextId;
    size_t id = _createRequestsNextId++;
    auto& request = _createRequests[id];
    auto future = request.get_future();

    rpc_stat_t e = nfs_create(&pfh, name.c_str(), &sattr, _createCallback, id);
    if (e != RPC_OK) {
        _createRequests.erase(id);
        _throwRpcError(e, "Failed to create a NFS file");
    }

    return future;
}

boost::future<void> remove(const fhandle_t& pfh, const std::string& name) {
    while (_removeRequests.count(_removeRequestsNextId) > 0)
        ++_removeRequestsNextId;
    size_t id = _removeRequestsNextId++;
    auto& request = _removeRequests[id];
    auto future = request.get_future();

    rpc_stat_t e = nfs_remove(&pfh, name.c_str(), _removeCallback, id);
    if (e != RPC_OK) {
        _removeRequests.erase(id);
        _throwRpcError(e, "Failed to remove a NFS file");
    }

    return future;
}

boost::future<fattr_t*> getattr(const fhandle_t& fh) {
    while (_getattrRequests.count(_getattrRequestsNextId) > 0)
        ++_getattrRequestsNextId;
    size_t id = _getattrRequestsNextId++;
    auto& request = _getattrRequests[id];
    auto future = request.get_future();

    rpc_stat_t e = nfs_getattr(&fh, _getattrCallback, id);
    if (e != RPC_OK) {
        _getattrRequests.erase(id);
        _throwRpcError(e, "Failed to get a NFS file's attributes");
    }

    return future;
}

boost::future<size_t> read(const fhandle_t& fh, off_t offset, size_t count, uint8_t* data) {
    if (count > std::numeric_limits<int>::max())
        throw std::invalid_argument("Count is bigger than what an int can store");

    while (_readRequests.count(_readRequestsNextId) > 0)
        ++_readRequestsNextId;
    size_t id = _readRequestsNextId++;
    auto& request = _readRequests[id] = {.handle = fh, .offset = offset, .count = count, .data = data};
    auto future = request.promise.get_future();

    rpc_stat_t e = nfs_read(&request.handle, offset, std::min(count, 6U*1026U), _readCallback, id);
    if (e != RPC_OK) {
        _readRequests.erase(id);
        _throwRpcError(e, "Failed to read from a NFS file");
    }

    return future;
}

boost::future<size_t> write(const fhandle_t& fh, off_t offset, size_t count, const uint8_t* data) {
    if (count > std::numeric_limits<int>::max())
        throw std::invalid_argument("Count is bigger than what an int can store");

    while (_writeRequests.count(_writeRequestsNextId) > 0)
        ++_writeRequestsNextId;
    size_t id = _writeRequestsNextId++;
    auto& request = _writeRequests[id] = {.handle = fh, .offset = offset, .count = count, .data = data};
    auto future = request.promise.get_future();

    rpc_stat_t e = nfs_write(&request.handle, offset, count, data, _writeCallback, id);
    if (e != RPC_OK) {
        _writeRequests.erase(id);
        _throwRpcError(e, "Failed to write to a NFS file");
    }

    return future;
}

}
