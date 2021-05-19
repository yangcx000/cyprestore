/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_EXTENTSERVER_BLOCK_DEVICE_H
#define CYPRESTORE_EXTENTSERVER_BLOCK_DEVICE_H

#include <cstdint>  // uint64_t
#include <memory>
#include <string>

#include "common/status.h"
#include "request_context.h"

namespace cyprestore {
namespace extentserver {

class BlockDevice;
typedef std::shared_ptr<BlockDevice> BlockDevicePtr;

enum BlockDeviceType {
    kTypeKernel,
    kTypeNVMe,
    kTypeUnknown = -1,
};

using common::Status;

class BlockDevice {
public:
    BlockDevice(const std::string &name, BlockDeviceType type)
            : name_(name), type_(type), capacity_(0), block_size_(0),
              write_unit_size_(0), align_size_(0) {}

    static BlockDeviceType ConvertTypeFromStr(const std::string &device_type);

    const std::string &name() {
        return name_;
    }
    BlockDeviceType type() {
        return type_;
    }

    uint64_t capacity() {
        return capacity_;
    }
    uint32_t block_size() {
        return block_size_;
    }
    uint32_t write_unit_size() {
        return write_unit_size_;
    }
    size_t align_size() {
        return align_size_;
    }

    virtual Status InitEnv() = 0;
    virtual Status CloseEnv() = 0;
    virtual Status Open() = 0;
    virtual Status Close() = 0;
    virtual Status PeriodDeviceAdmin() = 0;
    virtual Status ProcessRequest(Request *req) = 0;

    void dump();

protected:
    std::string name_;
    BlockDeviceType type_;
    /* 所有单位均为byte */
    uint64_t capacity_;
    uint32_t block_size_;
    uint32_t write_unit_size_;
    size_t align_size_;
};

}  // namespace extentserver
}  // namespace cyprestore

#endif
