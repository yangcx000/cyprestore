/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "file.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>

#include "butil/logging.h"

namespace cyprestore {
namespace clients {

int File::Open() {
    fd_ = open(file_path_.c_str(), O_CREAT | O_RDWR | O_SYNC, 0640);
    if (fd_ < 0) {
        LOG(ERROR) << "Couldn't open " << file_path_;
        return -1;
    }

    return ftruncate(fd_, capacity_);
}

int File::Write(uint64_t offset, uint64_t size, const char *buf) {
    ssize_t count = pwrite(fd_, buf, size, offset);
    if (count < 0) {
        LOG(ERROR) << "Couldn't write file"
                   << ", offset:" << offset << ", size:" << size
                   << ", file:" << file_path_;
        return -1;
    }

    assert(count == size);
    return 0;
}

int File::Read(uint64_t offset, uint64_t size, std::string &output) {
    ssize_t count =
            pread(fd_, const_cast<char *>(output.c_str()), size, offset);
    if (count < 0) {
        LOG(ERROR) << "Couldn't read file"
                   << ", offset:" << offset << ", size:" << size
                   << ", file:" << file_path_;
        return -1;
    }

    assert(count == size);
    return 0;
}

}  // namespace clients
}  // namespace cyprestore
