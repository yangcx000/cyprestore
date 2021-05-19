/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "nvme_device.h"

#include <butil/logging.h>

#include "common/config.h"
#include "common/cypre_ring.h"

namespace cyprestore {
namespace extentserver {

NVMeDevice::NVMeDevice(const std::string &name)
        : BlockDevice(name, BlockDeviceType::kTypeNVMe) {
    const common::SpdkCfg &spdk_cfg = GlobalConfig().spdk();
    SpdkEnvOptions env_options;
    env_options.shm_id = spdk_cfg.shm_id;
    env_options.mem_channel = spdk_cfg.mem_channel;
    env_options.mem_size = spdk_cfg.mem_size;
    env_options.master_core = spdk_cfg.master_core;
    env_options.num_pci_addr = spdk_cfg.num_pci_addr;
    env_options.no_pci = spdk_cfg.no_pci;
    env_options.hugepage_single_segments = spdk_cfg.hugepage_single_segments;
    env_options.unlink_hugepage = spdk_cfg.unlink_hugepage;
    env_options.core_mask = spdk_cfg.core_mask;
    env_options.huge_dir = spdk_cfg.huge_dir;
    env_options.name = spdk_cfg.name;
    env_options.json_config_file = spdk_cfg.json_config_file;
    env_options.rpc_addr = spdk_cfg.rpc_addr;

    spdk_mgr_.reset(new SpdkMgr(env_options));
}

Status NVMeDevice::InitEnv() {
    Status s = spdk_mgr_->InitEnv();
    if (!s.ok()) {
        LOG(ERROR) << "Couldn't init spdk enviroment, " << s.ToString();
    }
    return s;
}

Status NVMeDevice::CloseEnv() {
    Status s = spdk_mgr_->CloseEnv();
    if (!s.ok()) {
        LOG(ERROR) << "Couldn't close spdk enviroment, " << s.ToString();
        return s;
    }
    LOG(INFO) << "Close spdk environment.";
    return s;
}

Status NVMeDevice::Open() {
    Status s = spdk_mgr_->Open(name_);
    if (!s.ok()) {
        LOG(ERROR) << "Couldn't open spdk bdev " << name_ << ", "
                   << s.ToString();
        return s;
    }

    block_size_ = spdk_mgr_->GetBlockSize();
    uint64_t block_nums = spdk_mgr_->GetNumBlocks();
    capacity_ = block_size_ * block_nums;
    align_size_ = spdk_mgr_->GetBufAlignSize() * block_size_;
    write_unit_size_ = spdk_mgr_->GetWriteUnitSize() * block_size_;

    s = spdk_mgr_->StartWorkers();
    if (!s.ok()) {
        LOG(ERROR) << "Couldn't start worker threads, " << s.ToString();
        return s;
    }

    LOG(INFO) << "Open bdev " << name_ << ", capacity:" << capacity_
              << ", block_size:" << block_size_
              << ", align_size:" << align_size_
              << ", write_unit_size:" << write_unit_size_;

    return s;
}

Status NVMeDevice::Close() {
    Status s = spdk_mgr_->StopWorkers();
    if (!s.ok()) {
        LOG(ERROR) << "Couldn't stop worker threads, " << s.ToString();
        return s;
    }
    LOG(INFO) << "The worker threads have stopped";

    s = spdk_mgr_->Close(name_);
    if (!s.ok()) {
        LOG(ERROR) << "Couldn't close spdk bdev " << name_ << ", "
                   << s.ToString();
        return s;
    }
    LOG(INFO) << "The spdk bdev " << name_ << " has closed.";

    return s;
}

Status NVMeDevice::PeriodDeviceAdmin() {
    return spdk_mgr_->PeriodDeviceAdmin();
}

Status NVMeDevice::ProcessRequest(Request *req) {
    return spdk_mgr_->ProcessRequest(req);
}
}  // namespace extentserver
}  // namespace cyprestore
