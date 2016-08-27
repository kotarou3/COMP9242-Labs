#pragma once

namespace fs {

using uid=unsigned int;

class OpenFile {
    OpenFile(const std::string& fileName): fileName_{fileName} {}

    // no copying or moving open files. Purely pass by reference
    OpenFile(const OpenFile&) = delete;
    OpenFile& operator=(const OpenFile&) = delete;
    OpenFile(OpenFile&&) = delete;
    OpenFile& operator=(OpenFile&&) = delete;

    // some sort of connection to the file in the filesystem. filename for now
    std::string fileName_;

    friend class FileDescriptor;
};

}