#pragma once

#include "internal/fs/OpenFile.h"

namespace fs {

struct Mode {
    bool read:1, write:1;
};

// when a process is forked, the file descriptor table is copied.
// This makes multiple references to the one open file, which is behaviour we want

class FileDescriptor {
public:
    FileDescriptor(std::string, Mode);
private:
    std::shared_ptr<OpenFile> of;
    Mode mode;
};

class FDTable {
public:
    uid insert(FileDescriptor&);

    uid open(std::string, Mode);
    void close(uid);
private:
    std::unordered_map <uid, FileDescriptor> fds;
    uid upto;
};

}