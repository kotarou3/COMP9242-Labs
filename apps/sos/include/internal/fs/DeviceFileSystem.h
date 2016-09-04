#pragma once

#include <functional>
#include <map>
#include <string>

#include "internal/fs/File.h"

namespace fs {

class DeviceFileSystem : public FileSystem {
    public:
        virtual ~DeviceFileSystem() override = default;

        virtual boost::future<std::shared_ptr<File>> open(const std::string& pathname) override;
        virtual boost::future<std::unique_ptr<nfs::fattr_t>> stat(const std::string& pathname) override;

        using OpenCallback = std::function<boost::future<std::shared_ptr<File>> ()>;
        void create(const std::string& name, const OpenCallback& openCallback);

    private:
        std::map<std::string, OpenCallback> _devices;
};

}
