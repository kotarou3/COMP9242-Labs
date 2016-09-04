#pragma once

#include <nfs/nfs.h>

#include "internal/fs/File.h"
#include "internal/timer/timer.h"

namespace fs {

class NFSFileSystem : public FileSystem {
    public:
        NFSFileSystem(const std::string& serverIp, const std::string& nfsDir);
        virtual ~NFSFileSystem() override;

        virtual boost::future<std::shared_ptr<File>> open(const std::string& pathname) override;

    private:
        nfs::fhandle_t _handle;
        timer::TimerId _timeoutTimer;
};

}
