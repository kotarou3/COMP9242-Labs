#include <new>

#include "internal/fs/FileDescriptor.h"
#include "internal/fs/OpenFile.h"

namespace fs {

FileDescriptor::FileDescriptor(std::string fileName, Mode attr):
        of{make_shared<OpenFile>(filename, attr)} {}


uid FDTable::insert(FileDescriptor& fd) {
    while (fds.count(upto) > 0)
        ++upto;
    fds.emplace(upto, fd);
    return upto++;
}

uid FDTable::open(std::string fileName, Mode attr) {
    insert(FileDescriptor{fileName, attr});
}

void close(uid id) {
    fds.erase(id);
}

}
