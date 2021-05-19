/*
 * Copyright 2021 JDD authors.
 * @zhangliang
 *
 */

#ifndef CYPRESTORE_CLIENTS_MOCK_MOCK_LOGIC_H_
#define CYPRESTORE_CLIENTS_MOCK_MOCK_LOGIC_H_

#include "extentmanager/pb/resource.pb.h"
#include "extentmanager/pb/router.pb.h"
#include "extentserver/pb/extent_io.pb.h"

namespace cyprestore {
namespace clients {
namespace mock {

class MockUserLogic {
public:
    MockUserLogic() {}
    virtual ~MockUserLogic() {}

    // User
    virtual void CreateUser(
            const extentmanager::pb::CreateUserRequest *request,
            extentmanager::pb::CreateUserResponse *response) = 0;

    virtual void DeleteUser(
            const extentmanager::pb::DeleteUserRequest *request,
            extentmanager::pb::DeleteUserResponse *response) = 0;

    virtual void UpdateUser(
            const extentmanager::pb::UpdateUserRequest *request,
            extentmanager::pb::UpdateUserResponse *response) = 0;

    virtual void QueryUser(
            const extentmanager::pb::QueryUserRequest *request,
            extentmanager::pb::QueryUserResponse *response) = 0;

    virtual void ListUsers(
            const extentmanager::pb::ListUsersRequest *request,
            extentmanager::pb::ListUsersResponse *response) = 0;
};

class MockBlobLogic {
public:
    MockBlobLogic() {}
    virtual ~MockBlobLogic() {}

    // Blob
    virtual void CreateBlob(
            const extentmanager::pb::CreateBlobRequest *request,
            extentmanager::pb::CreateBlobResponse *response) = 0;

    virtual void DeleteBlob(
            const extentmanager::pb::DeleteBlobRequest *request,
            extentmanager::pb::DeleteBlobResponse *response) = 0;

    virtual void RenameBlob(
            const extentmanager::pb::RenameBlobRequest *request,
            extentmanager::pb::RenameBlobResponse *response) = 0;

    virtual void QueryBlob(
            const extentmanager::pb::QueryBlobRequest *request,
            extentmanager::pb::QueryBlobResponse *response) = 0;

    virtual void ListBlobs(
            const extentmanager::pb::ListBlobsRequest *request,
            extentmanager::pb::ListBlobsResponse *response) = 0;

    virtual void ResizeBlob(
            const extentmanager::pb::ResizeBlobRequest *request,
            extentmanager::pb::ResizeBlobResponse *response) = 0;

    virtual void QueryRouter(
            const extentmanager::pb::QueryRouterRequest *request,
            extentmanager::pb::QueryRouterResponse *response) = 0;
};

class MockExtentIoLogic {
public:
    MockExtentIoLogic() {}
    virtual ~MockExtentIoLogic() {}

    virtual void
    Read(google::protobuf::RpcController *cntl,
         const extentserver::pb::ReadRequest *request,
         extentserver::pb::ReadResponse *response) = 0;

    virtual void
    Write(google::protobuf::RpcController *cntl,
          const extentserver::pb::WriteRequest *request,
          extentserver::pb::WriteResponse *response) = 0;
};

const int DEF_EXTENT_SIZE = 32 * 1024 * 1024;  // 32M extent size for test
class GlobalConfig {
public:
    GlobalConfig() : extent_size_(DEF_EXTENT_SIZE){};

    static GlobalConfig *Instance() {
        static GlobalConfig gconf;
        return &gconf;
    }
    // set before service start
    void SetExtentSize(uint64_t size) {
        extent_size_ = (size & (~0x0FFF));
    }
    uint64_t GetExtentSize() const {
        return extent_size_;
    }

private:
    uint64_t extent_size_;
};

}  // namespace mock
}  // namespace clients
}  // namespace cyprestore

#endif  // CYPRESTORE_CLIENTS_UNITTEST_MOCK_MOCK_LOGIC_H_
