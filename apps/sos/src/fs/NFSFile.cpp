#include "internal/fs/NFSFile.h"

namespace fs {

NFSFile::NFSFile(fhandle_t* fh, fattr_t* attrs) {
    handle = *fh;
    this->attrs = *attrs;

}

}