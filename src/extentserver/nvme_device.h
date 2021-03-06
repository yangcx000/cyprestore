/*
 * Copyright (c) 2020 The Cyprestore Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. See the AUTHORS file for names of contributors.
 */

#ifndef CYPRESTORE_EXTENTSERVER_NVME_DEVICE_H
#define CYPRESTORE_EXTENTSERVER_NVME_DEVICE_H

#include <butil/macros.h>

#include <memory>
#include <string>

#include "butil/iobuf.h"
#include "extentserver/block_device.h"
#include "extentserver/io_mem.h"
#include "extentserver/spdk_mgr.h"

namespace cyprestore {
namespace extentserver {

class NVMeDevice : public BlockDevice {
public:
    NVMeDevice(const std::string &name);
    virtual ~NVMeDevice() = default;

    virtual Status InitEnv();
    virtual Status CloseEnv();
    virtual Status Open();
    virtual Status Close();
    virtual Status PeriodDeviceAdmin();
    virtual Status ProcessRequest(Request *req);

private:
    DISALLOW_COPY_AND_ASSIGN(NVMeDevice);

    std::unique_ptr<SpdkMgr> spdk_mgr_;
};

}  // namespace extentserver
}  // namespace cyprestore

#endif  // CYPRESTORE_EXTENTSERVER_NVME_DEVICE_H
