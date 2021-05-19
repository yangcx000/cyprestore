/*
 * Copyright 2021 JDD authors.
 * @zhangliang
 *
 */

#ifndef CYPRESTORE_CLIENTS_MOCK_MOCK_SERVICE_H_
#define CYPRESTORE_CLIENTS_MOCK_MOCK_SERVICE_H_

#include <brpc/channel.h>
#include <brpc/server.h>

#include <string>
#include <vector>

#include "mock/mock_logic.h"
#include "mock/mock_logic_impl.h"

namespace cyprestore {
namespace clients {
namespace mock {

class MockResourceService : public extentmanager::pb::ResourceService {
public:
    MockResourceService(MockUserLogic *ulogic, MockBlobLogic *blogic)
            : muser_(ulogic), mblob_(blogic) {}
    virtual ~MockResourceService() {}

    // User
    virtual void CreateUser(
            google::protobuf::RpcController *cntl_base,
            const extentmanager::pb::CreateUserRequest *request,
            extentmanager::pb::CreateUserResponse *response,
            google::protobuf::Closure *done) {
        brpc::ClosureGuard done_guard(done);
        muser_->CreateUser(request, response);
    }

    virtual void DeleteUser(
            google::protobuf::RpcController *cntl_base,
            const extentmanager::pb::DeleteUserRequest *request,
            extentmanager::pb::DeleteUserResponse *response,
            google::protobuf::Closure *done) {
        brpc::ClosureGuard done_guard(done);
        muser_->DeleteUser(request, response);
    }

    virtual void UpdateUser(
            google::protobuf::RpcController *cntl_base,
            const extentmanager::pb::UpdateUserRequest *request,
            extentmanager::pb::UpdateUserResponse *response,
            google::protobuf::Closure *done) {
        brpc::ClosureGuard done_guard(done);
        muser_->UpdateUser(request, response);
    }

    virtual void QueryUser(
            google::protobuf::RpcController *cntl_base,
            const extentmanager::pb::QueryUserRequest *request,
            extentmanager::pb::QueryUserResponse *response,
            google::protobuf::Closure *done) {
        brpc::ClosureGuard done_guard(done);
        muser_->QueryUser(request, response);
    }

    virtual void ListUsers(
            google::protobuf::RpcController *cntl_base,
            const extentmanager::pb::ListUsersRequest *request,
            extentmanager::pb::ListUsersResponse *response,
            google::protobuf::Closure *done) {
        brpc::ClosureGuard done_guard(done);
        muser_->ListUsers(request, response);
    }

    // Blob
    virtual void CreateBlob(
            google::protobuf::RpcController *cntl_base,
            const extentmanager::pb::CreateBlobRequest *request,
            extentmanager::pb::CreateBlobResponse *response,
            google::protobuf::Closure *done) {
        brpc::ClosureGuard done_guard(done);
        mblob_->CreateBlob(request, response);
    }

    virtual void DeleteBlob(
            google::protobuf::RpcController *cntl_base,
            const extentmanager::pb::DeleteBlobRequest *request,
            extentmanager::pb::DeleteBlobResponse *response,
            google::protobuf::Closure *done) {
        brpc::ClosureGuard done_guard(done);
        mblob_->DeleteBlob(request, response);
    }

    virtual void RenameBlob(
            google::protobuf::RpcController *cntl_base,
            const extentmanager::pb::RenameBlobRequest *request,
            extentmanager::pb::RenameBlobResponse *response,
            google::protobuf::Closure *done) {
        brpc::ClosureGuard done_guard(done);
        mblob_->RenameBlob(request, response);
    }

    virtual void QueryBlob(
            google::protobuf::RpcController *cntl_base,
            const extentmanager::pb::QueryBlobRequest *request,
            extentmanager::pb::QueryBlobResponse *response,
            google::protobuf::Closure *done) {
        brpc::ClosureGuard done_guard(done);
        mblob_->QueryBlob(request, response);
    }

    virtual void ListBlobs(
            google::protobuf::RpcController *cntl_base,
            const extentmanager::pb::ListBlobsRequest *request,
            extentmanager::pb::ListBlobsResponse *response,
            google::protobuf::Closure *done) {
        brpc::ClosureGuard done_guard(done);
        mblob_->ListBlobs(request, response);
    }

    virtual void ResizeBlob(
            google::protobuf::RpcController *cntl_base,
            const extentmanager::pb::ResizeBlobRequest *request,
            extentmanager::pb::ResizeBlobResponse *response,
            google::protobuf::Closure *done) {
        brpc::ClosureGuard done_guard(done);
        mblob_->ResizeBlob(request, response);
    }

private:
    MockUserLogic *muser_;
    MockBlobLogic *mblob_;
};

class MockRouterService : public extentmanager::pb::RouterService {
public:
    MockRouterService(MockBlobLogic *blogic) : mblob_(blogic) {}
    virtual ~MockRouterService() {}

    virtual void QueryRouter(
            google::protobuf::RpcController *cntl_base,
            const extentmanager::pb::QueryRouterRequest *request,
            extentmanager::pb::QueryRouterResponse *response,
            google::protobuf::Closure *done) {
        brpc::ClosureGuard done_guard(done);
        mblob_->QueryRouter(request, response);
    }

private:
    MockBlobLogic *mblob_;
};

class MockExtentIoService : public extentserver::pb::ExtentIOService {
public:
    MockExtentIoService(MockExtentIoLogic *io) : mio_(io) {}
    virtual ~MockExtentIoService() {}

    virtual void
    Read(google::protobuf::RpcController *cntl_base,
         const extentserver::pb::ReadRequest *request,
         extentserver::pb::ReadResponse *response,
         google::protobuf::Closure *done) {
        brpc::ClosureGuard done_guard(done);
        mio_->Read(cntl_base, request, response);
    }

    virtual void
    Write(google::protobuf::RpcController *cntl_base,
          const extentserver::pb::WriteRequest *request,
          extentserver::pb::WriteResponse *response,
          google::protobuf::Closure *done) {
        brpc::ClosureGuard done_guard(done);
        mio_->Write(cntl_base, request, response);
    }

private:
    MockExtentIoLogic *mio_;
};

class MockInstance {
public:
    MockInstance(MockLogicManager *mlm, MockExtentManager *mex);
    MockInstance();
    ~MockInstance();

    struct Address {
        Address(const std::string &i, int p) : ip(i), port(p) {}
        Address() : port(0) {}
        std::string ip;
        int port;
    };
    int StartMaster(const std::string &ip, int port);
    int SetRemoteServer(const std::vector<Address> &es);
    int StartServer(const std::vector<Address> &es);
    int Stop();

private:
    int startOneServer(const Address &addr);

    MockLogicManager *mlm_;
    MockExtentManager *mex_;
    MockUserLogic *ulogic_;
    MockBlobLogic *blogic_;
    MockExtentIoLogic *elogic_;

    brpc::Server master_server_;
    std::vector<brpc::Server *> es_servers_;
    volatile bool stop_;
};

class MockMasterClient {
public:
    MockMasterClient() {}
    ~MockMasterClient();
    // @endpoint, format: ip:port
    int Init(const std::string &endpoint);
    int CreateUser(
            const std::string &user_name, const std::string &email,
            std::string &user_id);
    int QueryUser(const std::string &user_id, common::pb::User &user);
    int ListUser(std::vector<common::pb::User> &llusers);

    int CreateBlob(
            const std::string &user_id, const std::string &blob_name,
            const std::string &blob_desc, common::pb::BlobType btype,
            size_t size, std::string &blob_id);
    int QueryBlob(const std::string &blob_id, common::pb::Blob &blob);
    int ListBlob(
            const std::string &user_id, std::vector<common::pb::Blob> &llblobs);

    int
    QueryRouter(const std::string &extent_id, common::pb::ExtentRouter &router);

    int
    Write(const std::string &blob_id, uint64_t offset, uint64_t len,
          const std::string &data);
    int
    Read(const std::string &blob_id, uint64_t offset, uint64_t len, char *buf);

private:
    int toExtent(
            const std::string &blob_id, uint64_t offset, uint64_t len,
            std::string &extent_id, uint64_t &exoffset);
    int initEsChannel(const std::string &extent_id, brpc::Channel *channel);

    brpc::Channel channel_;
};

}  // namespace mock
}  // namespace clients
}  // namespace cyprestore

#endif  // CYPRESTORE_CLIENTS_MOCK_MOCK_SERVICE_H_
