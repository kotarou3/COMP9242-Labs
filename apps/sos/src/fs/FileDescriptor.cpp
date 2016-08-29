#include <memory>
#include <new>

#include "internal/fs/FileDescriptor.h"
#include "internal/fs/File.h"

namespace fs {

FileDescriptor::FileDescriptor(std::shared_ptr<File> file, Mode attr):
        of{file}, mode{attr} {}


uid FDTable::insert(FileDescriptor fd) {
    while (fds.count(upto) > 0)
        ++upto;
    fds.emplace(upto, fd);
    return upto++;
}

void FDTable::close(uid id) {
    fds.erase(id);
}

}
