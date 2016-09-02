#include "internal/fs/NFSFile.h"

#define COPY(x, y) std::copy(&(x), (&(x)) + sizeof(x), y)

namespace fs {

NFSFile::NFSFile(fhandle_t* fh, fattr_t* fattr) {
    COPY(*fh, handle);
    COPY(fattr->atime, attrs.st_atime);
    COPY(fattr->ctime, attrs.st_ctime);
    attrs.st_fmode = (fattr->mode & S_IRUSR ? FM_READ : 0) |
                   (fattr->mode & S_IWUSR ? FM_WRITE : 0) |
                   (fattr->mode & S_IXUSR ? FM_EXEC : 0);
    attrs.st_size = fattr->size;
}

}