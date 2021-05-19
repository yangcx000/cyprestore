#include "mock_service.h"

#include <signal.h>

#include "common/error_code.h"
#include "common/extent_id_generator.h"

using namespace std;
using namespace cyprestore::clients::mock;

MockInstance::MockInstance(MockLogicManager *mlm, MockExtentManager *mex)
        : mlm_(mlm), mex_(mex) {
    ulogic_ = new MockUserLogicImpl(mlm_);
    blogic_ = new MockBlobLogicImpl(mlm_);
    elogic_ = new MockExtentIoLogicImpl(mex_);
}

MockInstance::MockInstance() {
    mlm_ = new MockLogicManager();
    mex_ = new MockMemExtentManager();
    ulogic_ = new MockUserLogicImpl(mlm_);
    blogic_ = new MockBlobLogicImpl(mlm_);
    elogic_ = new MockExtentIoLogicImpl(mex_);
}

MockInstance::~MockInstance() {
    if (!stop_) {
        Stop();
    }
}

int MockInstance::StartMaster(const std::string &ip, int port) {
    // register master's service
    MockRouterService *router_service = new MockRouterService(blogic_);
    if (master_server_.AddService(router_service, brpc::SERVER_OWNS_SERVICE)
        != 0) {
        LOG(ERROR) << "Add [RouterService] failed";
        return -1;
    }

    MockResourceService *resource_service =
            new MockResourceService(ulogic_, blogic_);
    if (master_server_.AddService(resource_service, brpc::SERVER_OWNS_SERVICE)
        != 0) {
        LOG(ERROR) << "Add [ResourceService] failed";
        return -1;
    }
    char endpoint[64];
    snprintf(endpoint, sizeof(endpoint) - 1, "%s:%d", ip.data(), port);
    brpc::ServerOptions options;
    options.num_threads = 32;
    options.max_concurrency = 0;
    if (master_server_.Start(endpoint, &options) != 0) {
        LOG(ERROR) << "Brpc server failed to start on " << endpoint;
        return -1;
    }
    signal(SIGPIPE, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    LOG(INFO) << "MockExtentManager server has started";
    stop_ = false;
    return 0;
}

int MockInstance::SetRemoteServer(const std::vector<Address> &es) {
    char buf[32];
    for (size_t i = 0; i < es.size(); i++) {
        snprintf(buf, sizeof(buf) - 1, "%d", es[i].port);
        mlm_->AddExtentServer(es[i].ip, buf);
    }
    return 0;
}

int MockInstance::StartServer(const std::vector<Address> &es) {
    for (size_t i = 0; i < es.size(); i++) {
        int rv = startOneServer(es[i]);
        if (rv != 0) {
            LOG(ERROR) << "start server failed:" << es[i].ip << ":"
                       << es[i].port;
            return rv;
        }
    }
    SetRemoteServer(es);
    return 0;
}

int MockInstance::Stop() {
    master_server_.Stop(0);
    ;
    master_server_.Join();
    for (size_t i = 0; i < es_servers_.size(); i++) {
        brpc::Server *server = es_servers_[i];
        server->Stop(0);
        server->Join();
    }
    for (size_t i = 0; i < es_servers_.size(); i++) {
        brpc::Server *server = es_servers_[i];
        delete server;
    }
    es_servers_.clear();
    stop_ = true;
    return 0;
}

int MockInstance::startOneServer(const Address &addr) {
    brpc::Server *server = new brpc::Server();
    // register service
    MockExtentIoService *io_service = new MockExtentIoService(elogic_);
    if (server->AddService(io_service, brpc::SERVER_OWNS_SERVICE) != 0) {
        LOG(ERROR) << "Add [ExtentIoService] failed";
        return -1;
    }
    char endpoint[64];
    snprintf(
            endpoint, sizeof(endpoint) - 1, "%s:%d", addr.ip.data(), addr.port);
    brpc::ServerOptions options;
    options.num_threads = 4;
    options.max_concurrency = 0;
    if (server->Start(endpoint, &options) != 0) {
        LOG(ERROR) << "Brpc server failed to start on " << endpoint;
        return -1;
    }
    es_servers_.push_back(server);
    return 0;
}

/////// Class MockMasterClient
MockMasterClient::~MockMasterClient() {}

int MockMasterClient::Init(const std::string &endpoint) {
    if (channel_.Init(endpoint.data(), NULL) != 0) {
        LOG(ERROR) << "Init channel failed, server:" << endpoint;
        return -1;
    }
    return 0;
}

int MockMasterClient::CreateUser(
        const std::string &user_name, const std::string &email,
        std::string &user_id) {
    // create a user
    brpc::Controller cntl;
    extentmanager::pb::ResourceService_Stub stub(&channel_);
    extentmanager::pb::CreateUserRequest request;
    request.set_user_name(user_name);
    request.set_email(email);
    request.set_comments("test user");
    extentmanager::pb::CreateUserResponse response;
    stub.CreateUser(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        LOG(ERROR) << "Create user failed:" << user_name << ","
                   << cntl.ErrorText();
        return -1;
    } else if (response.status().code() != 0) {
        LOG(ERROR) << "Create user failed:" << user_name
                   << ", error:" << response.status().code();
    } else {
        user_id = response.user_id();
        LOG(INFO) << "Create user :" << user_name << ", id:" << user_id;
    }
    return response.status().code();
}

int MockMasterClient::QueryUser(
        const std::string &user_id, common::pb::User &user) {
    brpc::Controller cntl;
    extentmanager::pb::ResourceService_Stub stub(&channel_);
    extentmanager::pb::QueryUserRequest request;
    request.set_user_id(user_id);
    extentmanager::pb::QueryUserResponse response;
    stub.QueryUser(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        LOG(ERROR) << "Query user failed:" << user_id << ","
                   << cntl.ErrorText();
        return -1;
    } else if (response.status().code() != 0) {
        LOG(ERROR) << "Query user failed:" << user_id
                   << ", error:" << response.status().code();
    } else {
        user = response.user();
    }
    return response.status().code();
}

int MockMasterClient::ListUser(std::vector<common::pb::User> &llusers) {
    brpc::Controller cntl;
    extentmanager::pb::ResourceService_Stub stub(&channel_);
    extentmanager::pb::ListUsersRequest request;
    extentmanager::pb::ListUsersResponse response;
    stub.ListUsers(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        LOG(ERROR) << "List user failed."
                   << "," << cntl.ErrorText();
        return -1;
    } else if (response.status().code() != 0) {
        LOG(ERROR) << "List user failed, error:" << response.status().code();
    } else {
        for (int i = 0; i < response.users_size(); i++) {
            llusers.push_back(response.users(i));
        }
    }
    return response.status().code();
}

int MockMasterClient::CreateBlob(
        const std::string &user_id, const std::string &blob_name,
        const std::string &blob_desc, common::pb::BlobType btype, size_t size,
        std::string &blob_id) {
    brpc::Controller cntl;
    extentmanager::pb::ResourceService_Stub stub(&channel_);
    extentmanager::pb::CreateBlobRequest request;
    request.set_user_id(user_id);
    request.set_blob_name(blob_name);
    request.set_blob_desc(blob_desc);
    request.set_blob_type(btype);
    request.set_instance_id("none");
    request.set_blob_size(size);
    extentmanager::pb::CreateBlobResponse response;
    stub.CreateBlob(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        LOG(ERROR) << "Create blob failed:" << blob_name << ","
                   << cntl.ErrorText();
        return -1;
    } else if (response.status().code() != 0) {
        LOG(ERROR) << "Create blob failed:" << blob_name
                   << ", error:" << response.status().code();
    } else {
        blob_id = response.blob_id();
        LOG(INFO) << "Create blob:" << blob_name << ", id:" << blob_id;
    }
    return response.status().code();
}

int MockMasterClient::QueryBlob(
        const std::string &blob_id, common::pb::Blob &blob) {
    brpc::Controller cntl;
    extentmanager::pb::ResourceService_Stub stub(&channel_);
    extentmanager::pb::QueryBlobRequest request;
    request.set_blob_id(blob_id);
    extentmanager::pb::QueryBlobResponse response;
    stub.QueryBlob(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        LOG(ERROR) << "Query blob failed:" << blob_id << ","
                   << cntl.ErrorText();
        return -1;
    } else if (response.status().code() != 0) {
        LOG(ERROR) << "Query blob failed:" << blob_id
                   << ", error:" << response.status().code();
    } else {
        blob = response.blob();
    }
    return response.status().code();
}

int MockMasterClient::ListBlob(
        const std::string &user_id, std::vector<common::pb::Blob> &llblobs) {
    brpc::Controller cntl;
    extentmanager::pb::ResourceService_Stub stub(&channel_);
    extentmanager::pb::ListBlobsRequest request;
    extentmanager::pb::ListBlobsResponse response;
    request.set_user_id(user_id);
    stub.ListBlobs(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        LOG(ERROR) << "List blob failed:" << user_id << "," << cntl.ErrorText();
        return -1;
    } else if (response.status().code() != 0) {
        LOG(ERROR) << "List blob failed:" << user_id
                   << ", error:" << response.status().code();
    } else {
        for (int i = 0; i < response.blobs_size(); i++) {
            llblobs.push_back(response.blobs(i));
        }
    }
    return response.status().code();
}

int MockMasterClient::QueryRouter(
        const std::string &extent_id, common::pb::ExtentRouter &router) {
    brpc::Controller cntl;
    extentmanager::pb::RouterService_Stub stub(&channel_);
    extentmanager::pb::QueryRouterRequest request;
    request.set_extent_id(extent_id);
    extentmanager::pb::QueryRouterResponse response;
    stub.QueryRouter(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        LOG(ERROR) << "Query router failed:" << extent_id << ","
                   << cntl.ErrorText();
        return -1;
    } else if (response.status().code() != 0) {
        LOG(ERROR) << "Query router failed:" << extent_id
                   << ", error:" << response.status().code();
    } else {
        router = response.router();
    }
    return response.status().code();
}

int MockMasterClient::toExtent(
        const std::string &blob_id, uint64_t offset, uint64_t len,
        std::string &extent_id, uint64_t &exoffset) {
    const uint64_t ext_size = GlobalConfig::Instance()->GetExtentSize();
    int index1 = offset / ext_size;
    int index2 = (offset + len - 1) / ext_size;
    if (index1 != index2) {
        LOG(ERROR) << "Write failed, can't cross extent, blob:" << blob_id;
        return common::CYPRE_ER_INVALID_ARGUMENT;
    }
    extent_id = common::ExtentIDGenerator::GenerateExtentID(blob_id, index1);
    exoffset = offset - index1 * ext_size;
    return common::CYPRE_OK;
}

int MockMasterClient::initEsChannel(
        const std::string &extent_id, brpc::Channel *channel) {
    common::pb::ExtentRouter router;
    int rv = QueryRouter(extent_id, router);
    if (rv != 0) {
        LOG(ERROR) << "query router failed, extent:" << extent_id
                   << ", return:" << rv;
        return rv;
    }
    const common::pb::EsInstance &es = router.primary();
    char endpoint[100];
    if (es.public_ip() == "0.0.0.0") {
        snprintf(
                endpoint, sizeof(endpoint) - 1, "127.0.0.1:%d",
                es.public_port());
    } else {
        snprintf(
                endpoint, sizeof(endpoint) - 1, "%s:%d", es.public_ip().data(),
                es.public_port());
    }
    if (channel->Init(endpoint, NULL) != 0) {
        LOG(ERROR) << "Init es channel failed, server:" << endpoint;
        return common::CYPRE_C_INTERNAL_ERROR;
    }
    return common::CYPRE_OK;
}

int MockMasterClient::Write(
        const std::string &blob_id, uint64_t offset, uint64_t len,
        const std::string &data) {
    std::string extent_id;
    uint64_t exoffset;
    int rv = toExtent(blob_id, offset, len, extent_id, exoffset);
    if (rv != common::CYPRE_OK) {
        return rv;
    }
    brpc::Channel channel;
    rv = initEsChannel(extent_id, &channel);
    if (rv != common::CYPRE_OK) {
        return rv;
    }
    butil::IOBuf iobuf;
    iobuf.append(data.data(), len);

    brpc::Controller cntl;
    extentserver::pb::ExtentIOService_Stub stub(&channel);
    extentserver::pb::WriteRequest request;
    request.set_extent_id(extent_id);
    request.set_offset(exoffset);
    request.set_size(len);
    cntl.request_attachment() = iobuf;

    extentserver::pb::WriteResponse response;
    stub.Write(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        LOG(ERROR) << "Write failed:" << extent_id << "," << cntl.ErrorText();
        return -1;
    } else if (response.status().code() != 0) {
        LOG(ERROR) << "Write failed:" << extent_id
                   << ", error:" << response.status().code();
    } else {
        LOG(INFO) << "Write ok:" << extent_id;
    }
    return response.status().code();
}

int MockMasterClient::Read(
        const std::string &blob_id, uint64_t offset, uint64_t len, char *buf) {
    std::string extent_id;
    uint64_t exoffset;
    int rv = toExtent(blob_id, offset, len, extent_id, exoffset);
    if (rv != 0) {
        return rv;
    }
    brpc::Channel channel;
    rv = initEsChannel(extent_id, &channel);
    if (rv != 0) {
        return rv;
    }
    brpc::Controller cntl;
    extentserver::pb::ExtentIOService_Stub stub(&channel);
    extentserver::pb::ReadRequest request;
    request.set_extent_id(extent_id);
    request.set_offset(exoffset);
    request.set_size(len);

    extentserver::pb::ReadResponse response;
    stub.Read(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        LOG(ERROR) << "Read failed:" << extent_id << "," << cntl.ErrorText();
        return -1;
    } else if (response.status().code() != 0) {
        LOG(ERROR) << "Read failed:" << extent_id
                   << ", error:" << response.status().code();
    } else {
        const butil::IOBuf &iobuf = cntl.response_attachment();
        iobuf.copy_to(buf, len);
        LOG(INFO) << "Read ok:" << extent_id;
    }
    return response.status().code();
}
