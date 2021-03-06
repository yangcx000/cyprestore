/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#include "block_device.h"

#include <butil/logging.h>

namespace cyprestore {
namespace extentserver {

BlockDeviceType
BlockDevice::ConvertTypeFromStr(const std::string &device_type) {
    if (device_type == "kernel")
        return BlockDeviceType::kTypeKernel;
    else if (device_type == "nvme") {
        return BlockDeviceType::kTypeNVMe;
    }

    return BlockDeviceType::kTypeUnknown;
}

void BlockDevice::dump() {
    LOG(INFO) << "\n*********************Block Device*********************"
              << "\nname:" << name_ << "\ntype:" << type_
              << "\ncapacity:" << capacity_ << "\nblock_size:"
              << block_size_
              //<< "\nnr_blocks:" << nr_blocks_
              << "\nalign_size:" << align_size_
              << "\nwrite_unit_size:" << write_unit_size_
              << "\n*********************Block Device*********************";
}

}  // namespace extentserver
}  // namespace cyprestore
