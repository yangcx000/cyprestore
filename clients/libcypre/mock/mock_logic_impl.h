/*
 * Copyright 2021 JDD authors.
 * @zhangliang
 *
 */

#ifndef CYPRESTORE_CLIENTS_MOCK_MOCK_LOGIC_IMPL_H_
#define CYPRESTORE_CLIENTS_MOCK_MOCK_LOGIC_IMPL_H_

#include <map>
#include <mutex>
#include <string>

#include "butil/iobuf.h"
#include "extentmanager/enum.h"
#include "mock/mock_logic.h"

namespace cyprestore {
namespace clients {
namespace mock {

struct UserData {
    std::string uid;
    std::string name;
    std::string email;
    std::string comments;
    std::string create_date;
    std::string update_date;
    std::vector<std::string> pool_ids;
    int set_id;

    void ToPbUser(common::pb::User &user) const;
};

struct BlobData {
    uint64_t size;
    extentmanager::BlobType type;
    std::string id;
    std::string name;
    std::string desc;
    std::string pool_id;
    std::string user_id;
    std::string instance_id;
    std::string create_date;
    std::string update_date;

    void ToPbBlob(common::pb::Blob &blob) const;
};

struct ExtentRouterData {
    std::string extent_id;
    std::string pool_id;
    std::string rg_id;
    std::string blob_id;
};

struct ESData {
    uint32_t id;
    std::string ip;
    uint32_t port;
};

class MockLogicManager {
public:
    MockLogicManager() : next_user_id_(0), next_blob_id_(0) {}
    virtual ~MockLogicManager() {}

    // Rgs
    virtual int
    AddExtentServer(const std::string &esip, const std::string &esport);

    // User
    virtual int CreateUser(UserData &user, std::string &user_id);
    virtual int DeleteUser(const std::string &user_id);
    virtual int UpdateUser(
            const std::string &user_id, const std::string &new_name,
            const std::string &new_email, const std::string &new_comments);
    virtual int QueryUser(const std::string &user_id, UserData *user);
    virtual int ListUsers(std::vector<UserData> &llusers);

    // Blob
    virtual int CreateBlob(
            const std::string &user_id, extentmanager::PoolType pool_type,
            const std::string &name, uint64_t size,
            extentmanager::BlobType type, const std::string &desc,
            const std::string &instance_id, std::string &blob_id);
    virtual int
    DeleteBlob(const std::string &user_id, const std::string &blob_id);
    virtual int RenameBlob(
            const std::string &user_id, const std::string &blob_id,
            const std::string &new_name);

    virtual int QueryBlob(const std::string &blob_id, bool all, BlobData *blob);
    virtual int
    ListBlobs(const std::string &user_id, std::vector<BlobData> &llblobs);
    virtual int ResizeBlob();

    // Router
    virtual int
    QueryRouter(const std::string &extent_id, std::string &rg_id, ESData &es);

protected:
    uint64_t next_user_id_;
    uint64_t next_blob_id_;
    std::map<std::string, UserData> users_;
    std::map<std::string, BlobData> blobs_;
    std::map<std::string, ESData> ess_;
    std::map<std::string, ExtentRouterData> extent_routers_;
    std::mutex lock_;
};

class MockExtentManager {
public:
    MockExtentManager() {}
    virtual ~MockExtentManager() {}
    virtual int
    Read(const std::string &extent_id, uint64_t offset, uint64_t len,
         butil::IOBuf &iobuf, extentserver::pb::ReadResponse *response) = 0;
    virtual int
    Write(const std::string &extent_id, uint64_t offset, uint64_t len,
          const butil::IOBuf &iobuf) = 0;
};

// do nothing extent io
class MockEmptyExtentManager : public MockExtentManager {
public:
    MockEmptyExtentManager() {}
    virtual ~MockEmptyExtentManager() {}
    virtual int
    Read(const std::string &extent_id, uint64_t offset, uint64_t len,
         butil::IOBuf &iobuf, extentserver::pb::ReadResponse *response);
    virtual int
    Write(const std::string &extent_id, uint64_t offset, uint64_t len,
          const butil::IOBuf &iobuf);
};

// memory extent io
class MockMemExtentManager : public MockExtentManager {
public:
    MockMemExtentManager() {}
    virtual ~MockMemExtentManager() {}
    virtual int
    Read(const std::string &extent_id, uint64_t offset, uint64_t len,
         butil::IOBuf &iobuf, extentserver::pb::ReadResponse *response);
    virtual int
    Write(const std::string &extent_id, uint64_t offset, uint64_t len,
          const butil::IOBuf &iobuf);

private:
    std::map<std::string, char *> extents_;
    std::mutex lock_;
};

class MockUserLogicImpl : public MockUserLogic {
public:
    MockUserLogicImpl(MockLogicManager *mgr) : mgr_(mgr) {}
    virtual ~MockUserLogicImpl() {}

    // User
    virtual void CreateUser(
            const extentmanager::pb::CreateUserRequest *request,
            extentmanager::pb::CreateUserResponse *response);

    virtual void DeleteUser(
            const extentmanager::pb::DeleteUserRequest *request,
            extentmanager::pb::DeleteUserResponse *response);

    virtual void UpdateUser(
            const extentmanager::pb::UpdateUserRequest *request,
            extentmanager::pb::UpdateUserResponse *response);

    virtual void QueryUser(
            const extentmanager::pb::QueryUserRequest *request,
            extentmanager::pb::QueryUserResponse *response);

    virtual void ListUsers(
            const extentmanager::pb::ListUsersRequest *request,
            extentmanager::pb::ListUsersResponse *response);

private:
    MockLogicManager *mgr_;
};

class MockBlobLogicImpl : public MockBlobLogic {
public:
    MockBlobLogicImpl(MockLogicManager *mgr) : mgr_(mgr) {}
    virtual ~MockBlobLogicImpl() {}

    // NOTE: Do not set pool_id in request, pool_type is needed.
    // There is one pool for each pool type in the mock logic implement
    virtual void CreateBlob(
            const extentmanager::pb::CreateBlobRequest *request,
            extentmanager::pb::CreateBlobResponse *response);

    virtual void DeleteBlob(
            const extentmanager::pb::DeleteBlobRequest *request,
            extentmanager::pb::DeleteBlobResponse *response);

    virtual void RenameBlob(
            const extentmanager::pb::RenameBlobRequest *request,
            extentmanager::pb::RenameBlobResponse *response);

    virtual void QueryBlob(
            const extentmanager::pb::QueryBlobRequest *request,
            extentmanager::pb::QueryBlobResponse *response);

    virtual void ListBlobs(
            const extentmanager::pb::ListBlobsRequest *request,
            extentmanager::pb::ListBlobsResponse *response);

    virtual void ResizeBlob(
            const extentmanager::pb::ResizeBlobRequest *request,
            extentmanager::pb::ResizeBlobResponse *response);

    virtual void QueryRouter(
            const extentmanager::pb::QueryRouterRequest *request,
            extentmanager::pb::QueryRouterResponse *response);

private:
    MockLogicManager *mgr_;
};

class MockExtentIoLogicImpl : public MockExtentIoLogic {
public:
    MockExtentIoLogicImpl(MockExtentManager *mgr) : exmgr_(mgr) {}
    virtual ~MockExtentIoLogicImpl() {}

    virtual void
    Read(google::protobuf::RpcController *cntl,
         const extentserver::pb::ReadRequest *request,
         extentserver::pb::ReadResponse *response);

    virtual void
    Write(google::protobuf::RpcController *cntl,
          const extentserver::pb::WriteRequest *request,
          extentserver::pb::WriteResponse *response);

private:
    MockExtentManager *exmgr_;
};

}  // namespace mock
}  // namespace clients
}  // namespace cyprestore

#endif  // CYPRESTORE_CLIENTS_UNITTEST_MOCK_MOCK_LOGIC_H_
