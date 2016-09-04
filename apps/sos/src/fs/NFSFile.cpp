#include "internal/fs/NFSFile.h"

namespace fs {

NFSFile::NFSFile(const nfs::fhandle_t* fh, const nfs::fattr_t* attrs) {
    handle = *fh;
    this->attrs = *attrs;

}

}