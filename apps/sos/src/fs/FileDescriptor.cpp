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

uid FDTable::open(std::string fileName, Mode attr) {
    std::shared_ptr<File> of;
    (void)fileName;
//    if (fileName == "console")
//        of = make_shared(ConsoleDevice());
    return insert(FileDescriptor(of, attr));
}

void FDTable::close(uid id) {
    fds.erase(id);
}

}
