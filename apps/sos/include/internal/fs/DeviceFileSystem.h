#pragma once

#include <functional>
#include <map>
#include <string>

#include "internal/fs/File.h"

namespace fs {

class DeviceFileSystem : public FileSystem {
    public:
        virtual ~DeviceFileSystem() override = default;

        virtual async::future<struct stat> stat(const std::string& pathname) override;
        virtual async::future<std::shared_ptr<File>> open(const std::string& pathname, OpenFlags flags) override;

        using OpenCallback = std::function<async::future<std::shared_ptr<File>> (OpenFlags flags)>;
        void create(const std::string& name, const OpenCallback& openCallback);

    private:
        struct Device {
            DeviceFileSystem::OpenCallback openCallback;

            timespec accessTime;
            timespec modifyTime;
            timespec changeTime;
        };
        std::map<std::string, Device> _devices;

        friend class DeviceDirectory;
};

class DeviceDirectory : public Directory {
    public:
        DeviceDirectory(const DeviceFileSystem& fs);
        virtual ~DeviceDirectory() override = default;

        virtual async::future<ssize_t> getdents(memory::UserMemory dirp, size_t length) override;

    private:
        const DeviceFileSystem& _fs;
        decltype(DeviceFileSystem::_devices)::const_iterator _currentPosition;

};

}
