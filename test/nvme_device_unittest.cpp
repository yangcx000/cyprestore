/*
 * Copyright 2021 JDD authors.
 * @zhangting361
 *
 */

#include "extentserver/nvme_device.h"

#include "butil/logging.h"
#include "common/arena.h"
#include "common/config.h"
#include "common/status.h"
#include "gtest/gtest.h"

namespace cyprestore {
namespace extentserver {
namespace {

using common::Status;
using extentserver::NVMeDevice;

class NVMeDeviceTest : public ::testing::Test {
protected:
    static void SetUpTestCase() {
        if (GlobalConfig().Init("extentserver", "./extentserver.ini") != 0) {
            std::cout << "Couldn't to init extentserver config" << std::endl;
            return;
        }

        nvme_device_ = new NVMeDevice(GlobalConfig().extentserver().dev_name);
    }

    static void TearDownTestCase() {
        delete nvme_device_;
    }

    void SetUp() override {
        std::cout << "SetUp: nvme_device init" << std::endl;
        auto status = nvme_device_->InitEnv();
        if (!status.ok()) {
            std::cout << "Init error" << std::endl;
        }
    }

    void TearDown() override {
        std::cout << "TearDown: nvme_device exit" << std::endl;
        auto status = nvme_device_->CloseEnv();
        if (!status.ok()) {
            std::cout << "Exit error" << std::endl;
        }
    }

    static NVMeDevice *nvme_device_;
};

NVMeDevice *NVMeDeviceTest::nvme_device_ = nullptr;
;

TEST_F(NVMeDeviceTest, TestOpen) {
    std::cout << "start to open nvme_device" << std::endl;
    auto status = nvme_device_->Open();
    ASSERT_TRUE(status.ok());

    sleep(5);

    std::cout << "start to close nvme_device" << std::endl;
    status = nvme_device_->Close();
    ASSERT_TRUE(status.ok());
}
}  // namespace
}  // namespace extentserver
}  // namespace cyprestore
