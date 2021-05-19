/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 */

#include "safe_io.h"

#include "util.h"

namespace cyprestore {
namespace nbd {

ssize_t SafeIO::ReadExact(int fd, void *buf, size_t count) {
    return SafeReadExact(fd, buf, count);
}

ssize_t SafeIO::Read(int fd, void *buf, size_t count) {
    return SafeRead(fd, buf, count);
}

ssize_t SafeIO::Write(int fd, const void *buf, size_t count) {
    return SafeWrite(fd, buf, count);
}

}  // namespace nbd
}  // namespace cyprestore
