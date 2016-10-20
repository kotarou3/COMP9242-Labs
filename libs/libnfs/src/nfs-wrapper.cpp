#include <limits>
#include <stdexcept>
#include <system_error>
#include <unordered_map>

#include <boost/functional/hash.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/sequenced_index.hpp>

#include <cstring>
#include <assert.h>
#include <limits.h>

extern "C" {
    #include <lwip/pbuf.h>
}

#include "nfs.h"

namespace nfs {

constexpr bool operator==(const fhandle_t& a, const fhandle_t& b) {
    return !std::memcmp(a.data, b.data, FHSIZE);
}

size_t hash_value(const fhandle_t& a) {
    return boost::hash_range(a.data, a.data + FHSIZE);
}

namespace {
    constexpr const bool ENABLE_CACHING = true;
    constexpr const size_t MAX_CACHE_ENTRIES = 50;
    constexpr const size_t MAX_CACHE_DATA_SIZE = 10 * 1024 * 1024;
    constexpr const size_t MAX_CACHE_DATA_WRITE_MISSES = 4;

    struct CacheEntry {
        fhandle_t handle;

        fattr_t attributes;
        bool isAttributesValid = false;

        std::vector<std::string> readdirResult;
        bool isReaddirResultValid = false;

        // Simple cache for forward linear accesses
        std::vector<uint8_t> data;
        off_t dataOffset;
        size_t dataWriteMisses = 0;
        bool isDataValid = false;
        size_t dirtyStart = 0;
        size_t dirtyEnd = 0;

        CacheEntry(const CacheEntry&) = delete;
        CacheEntry& operator=(const CacheEntry&) = delete;

        CacheEntry(CacheEntry&&) = default;
        CacheEntry& operator=(CacheEntry&&) = default;

        void flush() {
            if (!isDataValid || dirtyStart == dirtyEnd)
                return;

            // XXX: We assume always-successful NFS ops for caching, so no
            // error handling is performed
            for (size_t offset = dirtyStart; offset < dirtyEnd; offset += NFS_WRITE_BLOCK_SIZE) {
                nfs_write(
                    &handle,
                    dataOffset + offset,
                    std::min(dirtyEnd - offset, NFS_WRITE_BLOCK_SIZE),
                    data.data() + offset,
                    [](size_t, nfs_stat_t, fattr_t*, int) {}, 0
                );
            }

            // nfs_write() copies the buffer to it's own buffer, so we are free
            // to trash the buffer afterwards
            dirtyStart = dirtyEnd = 0;
        }
    };

    using namespace boost::multi_index;
    class Cache : public multi_index_container<
        CacheEntry,
        indexed_by<
            sequenced<>,
            hashed_unique<member<CacheEntry, fhandle_t, &CacheEntry::handle>>
        >
    > {
        public:
            iterator insert(const fhandle_t& handle) {
                auto result = push_front(CacheEntry{.handle = handle});

                if (!result.second) {
                    // Already in cache - move to front
                    relocate(begin(), result.first);
                    return begin();
                } else if (size() > MAX_CACHE_ENTRIES) {
                    // Cache full - remove LRU entry
                    if (back().isDataValid && back().dirtyStart != back().dirtyEnd)
                        modify(--end(), [](auto& entry) {entry.flush();});
                    pop_back();
                }

                return result.first;
            }

            iterator find(const fhandle_t& handle) noexcept {
                auto result = get<1>().find(handle);
                if (result != get<1>().end()) {
                    relocate(begin(), project<0>(result));
                    return begin();
                } else {
                    return end();
                }
            }

            size_t read(const fhandle_t& handle, off_t offset, size_t length, uint8_t* to) {
                auto entry = find(handle);
                if (entry == end() || !entry->isDataValid)
                    return false;

                switch (_classifyOverlap(offset, length, *entry)) {
                    case OverlapType::Complete:
                        if (offset != entry->dataOffset || length != entry->data.size())
                            break;
                        // Fallthrough

                    case OverlapType::Middle: {
                        size_t overlapStart = offset - entry->dataOffset;
                        std::copy(entry->data.begin() + overlapStart, entry->data.begin() + overlapStart + length, to);
                        return length;
                    }

                    case OverlapType::End: {
                        size_t overlapStart = offset - entry->dataOffset;
                        size_t overlapSize = entry->data.size() - overlapStart;
                        std::copy(entry->data.begin() + overlapStart, entry->data.end(), to);
                        return overlapSize;
                    }

                    case OverlapType::None:
                        return false;

                    default:
                        break;
                }

                if (entry->dirtyStart != entry->dirtyEnd)
                    modify(entry, [](auto& entry) {entry.flush();});
                return false;
            }

            bool write(const fhandle_t& handle, off_t offset, size_t length, const uint8_t* from, bool markDataDirty) {
                static_assert(MAX_CACHE_DATA_SIZE > 2 * NFS_WRITE_BLOCK_SIZE, "Cache data size must be at least twice larger than NFS write block size");
                if (length > NFS_WRITE_BLOCK_SIZE)
                    return false;

                auto entry = insert(handle);

                if (!entry->isDataValid || entry->dataWriteMisses >= MAX_CACHE_DATA_WRITE_MISSES) {
                    modify(entry, [=](auto& entry) {
                        if (entry.isDataValid && entry.dirtyStart != entry.dirtyEnd)
                            entry.flush();

                        entry.data.clear();
                        entry.data.insert(entry.data.begin(), from, from + length);
                        entry.dataOffset = offset;
                        entry.dataWriteMisses = 0;
                        entry.isDataValid = true;
                        if (markDataDirty) {
                            entry.dirtyStart = 0;
                            entry.dirtyEnd = entry.data.size();
                        } else {
                            entry.dirtyStart = entry.dirtyEnd = 0;
                        }
                    });

                    return true;
                }

                switch (_classifyOverlap(offset, length, *entry)) {
                    case OverlapType::Complete:
                        modify(entry, [=](auto& entry) {
                            entry.data.clear();
                            entry.data.insert(entry.data.begin(), from, from + length);
                            entry.dataOffset = offset;
                            entry.dataWriteMisses = 0;

                            if (markDataDirty) {
                                entry.dirtyStart = 0;
                                entry.dirtyEnd = entry.data.size();
                            } else {
                                assert(entry.dirtyStart == entry.dirtyEnd);
                            }
                        });
                        break;

                    case OverlapType::Start:
                        modify(entry, [=](auto& entry) {
                            size_t overlapSize = offset + length - entry.dataOffset;
                            size_t newSize = length + entry.data.size() - overlapSize;
                            assert(newSize <= MAX_CACHE_DATA_SIZE);
                            entry.data.resize(newSize);

                            std::copy(entry.data.begin() + overlapSize, entry.data.end(), entry.data.begin() + length);
                            std::copy(from, from + length, entry.data.begin());
                            entry.dataOffset = offset;
                            entry.dataWriteMisses = 0;

                            if (markDataDirty) {
                                if (entry.dirtyStart < overlapSize) {
                                    entry.dirtyStart = 0;
                                    entry.dirtyEnd += length - overlapSize;
                                } else {
                                    entry.flush();
                                    entry.dirtyStart = 0;
                                    entry.dirtyEnd = length;
                                }
                            } else {
                                if (entry.dirtyStart >= overlapSize) {
                                    entry.dirtyStart += length - overlapSize;
                                    entry.dirtyEnd += length - overlapSize;
                                } else {
                                    assert(entry.dirtyStart == entry.dirtyEnd);
                                }
                            }
                        });
                        break;

                    case OverlapType::Middle:
                        modify(entry, [=](auto& entry) {
                            size_t overlapStart = offset - entry.dataOffset;
                            std::copy(from, from + length, entry.data.begin() + overlapStart);
                            entry.dataWriteMisses = 0;

                            auto dirtyOverlap = _classifyOverlap(overlapStart, overlapStart + length, entry.dirtyStart, entry.dirtyEnd);
                            if (markDataDirty) {
                                switch (dirtyOverlap) {
                                    case OverlapType::Complete:
                                        entry.dirtyStart = overlapStart;
                                        entry.dirtyEnd = overlapStart + length;
                                        break;

                                    case OverlapType::Start:
                                        entry.dirtyStart = overlapStart;
                                        break;

                                    case OverlapType::Middle:
                                        break;

                                    case OverlapType::End:
                                        entry.dirtyEnd = overlapStart + length;
                                        break;

                                    case OverlapType::None:
                                        entry.flush();
                                        entry.dirtyStart = overlapStart;
                                        entry.dirtyEnd = overlapStart + length;
                                        break;
                                }
                            } else {
                                assert(dirtyOverlap == OverlapType::None);
                            }
                        });
                        break;

                    case OverlapType::None:
                        if (entry->dataOffset + entry->data.size() != offset) {
                            modify(entry, [=](auto& entry) {
                                ++entry.dataWriteMisses;
                            });
                            return false;
                        }
                        // Fallthrough

                    case OverlapType::End:
                        modify(entry, [=](auto& entry) {
                            size_t overlapStart = offset - entry.dataOffset;
                            size_t newSize = overlapStart + length;
                            assert(newSize <= MAX_CACHE_DATA_SIZE);
                            entry.data.resize(newSize);

                            std::copy(from, from + length, entry.data.begin() + overlapStart);
                            entry.dataWriteMisses = 0;

                            if (markDataDirty) {
                                if (entry.dirtyEnd >= overlapStart) {
                                    entry.dirtyEnd = overlapStart + length;
                                } else {
                                    entry.flush();
                                    entry.dirtyStart = overlapStart;
                                    entry.dirtyEnd = overlapStart + length;
                                }
                            } else {
                                if (entry.dirtyEnd < overlapStart) {
                                    // Do nothing
                                } else {
                                    assert(entry.dirtyStart == entry.dirtyEnd);
                                }
                            }
                        });
                        break;
                }

                if (entry->dirtyEnd - entry->dirtyStart >= NFS_WRITE_BLOCK_SIZE)
                    modify(entry, [=](auto& entry) {entry.flush();});
                if (entry->data.size() >= MAX_CACHE_DATA_SIZE - NFS_WRITE_BLOCK_SIZE)
                    modify(entry, [=](auto& entry) {entry.data.erase(entry.data.begin(), entry.data.end() - NFS_WRITE_BLOCK_SIZE);});

                return true;
            }

        private:
            enum class OverlapType {None, Complete, Start, Middle, End};
            static OverlapType _classifyOverlap(off_t ofStart, off_t ofEnd, off_t onStart, off_t onEnd) noexcept {
                bool startsBefore = ofStart <= onStart;
                bool startsInside = onStart < ofStart && ofStart < onEnd;
                bool endsAfter = onEnd <= ofEnd;
                bool endsInside = onStart < ofEnd && ofEnd < onEnd;

                if (startsBefore && endsAfter)
                    return OverlapType::Complete;
                else if (startsBefore && endsInside)
                    return OverlapType::Start;
                else if (startsInside && endsInside)
                    return OverlapType::Middle;
                else if (startsInside && endsAfter)
                    return OverlapType::End;
                else
                    return OverlapType::None;
            }
            static OverlapType _classifyOverlap(off_t offset, size_t length, const CacheEntry& entry) noexcept {
                assert(entry.isDataValid);

                off_t start = offset;
                off_t end = start + length;
                off_t entryStart = entry.dataOffset;
                off_t entryEnd = entry.dataOffset + entry.data.size();

                return _classifyOverlap(start, end, entryStart, entryEnd);
            }
    } _cache;

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
            if (ENABLE_CACHING) {
                try {
                    _cache.modify(_cache.insert(request.handle), [&](auto& entry) {
                        entry.readdirResult = request.result;
                        entry.isReaddirResultValid = true;
                    });
                } catch (...) {
                    // Ignore cache errors
                }
            }

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

    struct CreateRequest {
        fhandle_t handle;
        std::string filename;
        boost::promise<std::pair<const fhandle_t*, fattr_t*>> promise;
    };
    std::unordered_map<size_t, CreateRequest> _createRequests;
    size_t _createRequestsNextId;
    void _createCallback(size_t id, nfs_stat_t e, fhandle_t* handle, fattr_t* attr) noexcept try {
        if (e != NFS_OK)
            _throwNfsError(e, "Failed to create a NFS file");

        auto& request = _createRequests.at(id);

        if (ENABLE_CACHING) {
            try {
                auto dirEntry = _cache.find(request.handle);
                if (dirEntry != _cache.end() && dirEntry->isReaddirResultValid) {
                    _cache.modify(dirEntry, [&](auto& dirEntry) {
                        dirEntry.readdirResult.push_back(std::move(request.filename));
                    });
                }

                _cache.modify(_cache.insert(*handle), [&](auto& fileEntry) {
                    fileEntry.attributes = *attr;
                    fileEntry.isAttributesValid = true;
                });
            } catch (...) {
                // Ignore cache errors
            }
        }

        request.promise.set_value(std::make_pair(handle, attr));
        _createRequests.erase(id);
    } catch (...) {
        _createRequests.at(id).promise.set_exception(std::current_exception());
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

    struct GetattrRequest {
        fhandle_t handle;
        boost::promise<const fattr_t*> promise;
    };
    std::unordered_map<size_t, GetattrRequest> _getattrRequests;
    size_t _getattrRequestsNextId;
    void _getattrCallback(size_t id, nfs_stat_t e, fattr_t* attr) noexcept try {
        if (e != NFS_OK)
            _throwNfsError(e, "Failed to get a NFS file's attributes");

        auto& request = _getattrRequests.at(id);

        if (ENABLE_CACHING) {
            try {
                _cache.modify(_cache.insert(request.handle), [&](auto& entry) {
                    entry.attributes = *attr;
                    entry.isAttributesValid = true;
                });
            } catch (...) {
                // Ignore cache errors
            }
        }

        request.promise.set_value(attr);
        _getattrRequests.erase(id);
    } catch (...) {
        _getattrRequests.at(id).promise.set_exception(std::current_exception());
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
    void _readCallback(size_t id, nfs_stat_t e, fattr_t* /*attr*/, pbuf* packet, int length, int pos) noexcept try {
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

        if (length > request.count) {
            // Handle readahead
            if (ENABLE_CACHING) {
                try {
                    std::vector<uint8_t> buffer(length);
                    pbuf_copy_partial(packet, buffer.data(), length, pos);
                    _cache.write(request.handle, request.offset + request.readBytes, length, buffer.data(), false);
                } catch (...) {
                    // Ignore cache errors
                }
            }

            pbuf_copy_partial(packet, request.data + request.readBytes, request.count - request.readBytes, pos);
            request.readBytes = request.count;

            goto resolvePromise;
        } else {
            pbuf_copy_partial(packet, request.data + request.readBytes, length, pos);
            request.readBytes += length;
        }

        if (length > 0 && request.readBytes < request.count) {
            rpc_stat_t e = nfs_read(
                &request.handle,
                request.offset + request.readBytes,
                request.count - request.readBytes,
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
    if (ENABLE_CACHING) {
        auto entry = _cache.find(pfh);
        if (entry != _cache.end() && entry->isReaddirResultValid) {
            boost::promise<std::vector<std::string>> promise;
            promise.set_value(entry->readdirResult);
            return promise.get_future();
        }
    }

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
    auto& request = _createRequests[id] = {.handle = pfh, .filename = name};
    auto future = request.promise.get_future();

    rpc_stat_t e = nfs_create(&pfh, name.c_str(), &sattr, _createCallback, id);
    if (e != RPC_OK) {
        _createRequests.erase(id);
        _throwRpcError(e, "Failed to create a NFS file");
    }

    return future;
}

boost::future<void> remove(const fhandle_t& pfh, const std::string& name) {
    if (ENABLE_CACHING) {
        auto entry = _cache.find(pfh);
        if (entry != _cache.end() && entry->isReaddirResultValid)
            _cache.modify(entry, [](auto& entry) {entry.isReaddirResultValid = false;});
    }

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

boost::future<const fattr_t*> getattr(const fhandle_t& fh) {
    if (ENABLE_CACHING) {
        auto entry = _cache.find(fh);
        if (entry != _cache.end() && entry->isAttributesValid)
            return boost::make_ready_future<const fattr_t*>(&entry->attributes);
    }

    while (_getattrRequests.count(_getattrRequestsNextId) > 0)
        ++_getattrRequestsNextId;
    size_t id = _getattrRequestsNextId++;
    auto& request = _getattrRequests[id] = {.handle = fh};
    auto future = request.promise.get_future();

    rpc_stat_t e = nfs_getattr(&fh, _getattrCallback, id);
    if (e != RPC_OK) {
        _getattrRequests.erase(id);
        _throwRpcError(e, "Failed to get a NFS file's attributes");
    }

    return future;
}

boost::future<size_t> read(const fhandle_t& fh, off_t offset, size_t count, uint8_t* data, bool isCacheable) {
    if (count > std::numeric_limits<int>::max())
        throw std::invalid_argument("Count is bigger than what an int can store");
    isCacheable &= ENABLE_CACHING;

    size_t cacheRead = 0;
    if (isCacheable) {
        cacheRead = _cache.read(fh, offset, count, data);
        if (cacheRead == count)
            return boost::make_ready_future<size_t>(count);
    }

    while (_readRequests.count(_readRequestsNextId) > 0)
        ++_readRequestsNextId;
    size_t id = _readRequestsNextId++;
    auto& request = _readRequests[id] = {.handle = fh, .offset = offset, .count = count, .data = data, .readBytes = cacheRead};
    auto future = request.promise.get_future();

    size_t toRead = isCacheable ? std::max(count - cacheRead, NFS_READ_BLOCK_SIZE) : count - cacheRead;
    rpc_stat_t e = nfs_read(&request.handle, offset + cacheRead, toRead, _readCallback, id);
    if (e != RPC_OK) {
        _readRequests.erase(id);
        _throwRpcError(e, "Failed to read from a NFS file");
    }

    return future;
}

boost::future<size_t> write(const fhandle_t& fh, off_t offset, size_t count, const uint8_t* data, bool isCacheable) {
    if (count > std::numeric_limits<int>::max())
        throw std::invalid_argument("Count is bigger than what an int can store");
    isCacheable &= ENABLE_CACHING;

    if (isCacheable && _cache.write(fh, offset, count, data, true))
        return boost::make_ready_future<size_t>(count);

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

void flush(const fhandle_t& fh) {
    if (ENABLE_CACHING) {
        auto entry = _cache.find(fh);
        if (entry != _cache.end() && entry->isDataValid)
            _cache.modify(entry, [](auto& entry) {entry.flush();});
    }
}

}
