#pragma once

#include <unordered_map>
#include <memory>

namespace fs {

using uid=unsigned int;

struct Mode {
    bool read:1, write:1, execute:1;
};

// when a process is forked, the file descriptor table is copied.
// This makes multiple references to the one open file, which is behaviour we want

class File;

class FileDescriptor {
public:
    FileDescriptor(std::shared_ptr<File>, Mode);
private:
    std::shared_ptr<File> of;
    Mode mode;
};

class FDTable {
public:
    uid insert(FileDescriptor);

    void close(uid);
private:
    std::unordered_map <uid, FileDescriptor> fds;
    uid upto;
};

}