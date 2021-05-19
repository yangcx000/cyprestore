/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 */

#ifndef NBD_SRC_SAFEIO_H_
#define NBD_SRC_SAFEIO_H_

#include <cstddef>
#include <cstdio>

namespace cyprestore {
namespace nbd {

// 封装SafeRead/write接口
class SafeIO {
public:
    SafeIO() = default;
    virtual ~SafeIO() = default;

    virtual ssize_t ReadExact(int fd, void *buf, size_t count);
    virtual ssize_t Read(int fd, void *buf, size_t count);
    virtual ssize_t Write(int fd, const void *buf, size_t count);
};

}  // namespace nbd
}  // namespace cyprestore

#endif  // NBD_SRC_SAFEIO_H_
