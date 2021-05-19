/*
 * Copyright 2021 JDD authors.
 * @zhangliang
 *
 */

#include "mock/mock_logic_impl.h"

#include "brpc/controller.h"
#include "butil/logging.h"
#include "common/error_code.h"
#include "common/extent_id_generator.h"
#include "utils/chrono.h"
#include "utils/crc32.h"
#include "utils/pb_transfer.h"

using namespace std;
using namespace cyprestore::clients::mock;

const int SETID = 101;
const int POOLNUM = 4;
const int MAX_BLOCK_SIZE = 32 * 1024 * 1024;  // 128M max block size for test
const string MPOOLIDS[POOLNUM] = { "mpool-hdd", "mpool-ssd", "mpool-nvme",
                                   "mpool-spec" };

template <class T> static std::string vec2str(const vector<T> &vecs) {
    std::stringstream ss(std::ios_base::app | std::ios_base::out);
    for (auto &v : vecs) {
        ss << v << ", ";
    }
    return ss.str();
}

void UserData::ToPbUser(common::pb::User &user) const {
    user.set_id(uid);
    user.set_name(name);
    user.set_email(email);
    user.set_comments(comments);
    user.set_set_id(set_id);
    user.set_status(common::pb::USER_STATUS_CREATED);
    user.set_pool_id(vec2str<string>(pool_ids));
    user.set_create_date(create_date);
    user.set_update_date(update_date);
}

void BlobData::ToPbBlob(common::pb::Blob &blob) const {
    blob.set_id(id);
    blob.set_name(name);
    blob.set_size(size);
    blob.set_type(utils::toPbBlobType(type));
    blob.set_desc(desc);
    blob.set_status(common::pb::BLOB_STATUS_CREATED);
    blob.set_pool_id(pool_id);
    blob.set_user_id(user_id);
    blob.set_instance_id(instance_id);
    blob.set_create_date(create_date);
    blob.set_update_date(update_date);
}

////// Class MockLogicManager
int MockLogicManager::AddExtentServer(
        const std::string &esip, const std::string &esport) {
    std::string rgid = esip + "." + esport;
    std::lock_guard<std::mutex> guard(lock_);
    if (ess_.count(rgid) > 0) {
        LOG(ERROR) << "Duplicate es server: " << rgid;
        return common::CYPRE_ER_INVALID_ARGUMENT;
    }
    ESData es;
    es.id = ess_.size() + 1;
    es.ip = esip;
    es.port = atoi(esport.data());
    ess_[rgid] = es;
    return common::CYPRE_OK;
}

int MockLogicManager::CreateUser(UserData &user, std::string &user_id) {
    char idbuf[64] = "";
    std::lock_guard<std::mutex> guard(lock_);
    snprintf(idbuf, sizeof(idbuf) - 1, "muser%lu", next_user_id_);
    user_id = idbuf;
    for (auto iter = users_.begin(); iter != users_.end(); ++iter) {
        if (user.name == iter->second.name) {
            LOG(ERROR) << "Duplicate user: " << user.name;
            return common::CYPRE_EM_USER_DUPLICATED;
        }
    }
    user.uid = user_id;
    next_user_id_++;
    users_[user_id] = user;
    LOG(INFO) << "Create user succeed, username: " << user.name
              << ", userid: " << user_id;
    return common::CYPRE_OK;
}

int MockLogicManager::DeleteUser(const std::string &user_id) {
    std::lock_guard<std::mutex> guard(lock_);
    auto iter = users_.find(user_id);
    if (iter == users_.end()) {
        LOG(ERROR) << "Not find user, id: " << user_id;
        return common::CYPRE_EM_USER_NOT_FOUND;
    }
    users_.erase(iter);
    LOG(INFO) << "Delete user succeed, user_id:" << user_id;
    return common::CYPRE_OK;
}

int MockLogicManager::UpdateUser(
        const std::string &user_id, const std::string &new_name,
        const std::string &new_email, const std::string &new_comments) {
    std::lock_guard<std::mutex> guard(lock_);
    auto iter = users_.find(user_id);
    if (iter == users_.end()) {
        LOG(ERROR) << "Not find user, id: " << user_id;
        return common::CYPRE_EM_USER_NOT_FOUND;
    }
    if (!new_name.empty()) {
        iter->second.name = new_name;
    }
    if (!new_email.empty()) {
        iter->second.email = new_email;
    }
    if (!new_comments.empty()) {
        iter->second.comments = new_comments;
    }
    return common::CYPRE_OK;
}

int MockLogicManager::QueryUser(const std::string &user_id, UserData *user) {
    std::lock_guard<std::mutex> guard(lock_);
    auto iter = users_.find(user_id);
    if (iter == users_.end()) {
        LOG(ERROR) << "Not find user, id: " << user_id;
        return common::CYPRE_EM_USER_NOT_FOUND;
    }
    if (user != NULL) {
        *user = iter->second;
    }
    LOG(INFO) << "Query user succeed, user_id:" << user_id;
    return common::CYPRE_OK;
}

int MockLogicManager::ListUsers(vector<UserData> &llusers) {
    std::lock_guard<std::mutex> guard(lock_);
    for (auto iter = users_.begin(); iter != users_.end(); ++iter) {
        llusers.push_back(iter->second);
    }
    return common::CYPRE_OK;
}

int MockLogicManager::CreateBlob(
        const std::string &user_id, extentmanager::PoolType pool_type,
        const std::string &name, uint64_t size, extentmanager::BlobType type,
        const std::string &desc, const std::string &instance_id,
        std::string &blob_id) {
    if (pool_type == extentmanager::PoolType::kPoolTypeUnknown) {
        LOG(ERROR) << "Invalid pool type: " << pool_type;
        return common::CYPRE_ER_INVALID_ARGUMENT;
    }
    uint64_t ext_size = GlobalConfig::Instance()->GetExtentSize();
    if (size < ext_size) {
        LOG(ERROR) << "Invalid size: " << size;
        return common::CYPRE_ER_INVALID_ARGUMENT;
    }
    const size_t extents = (size - 1) / ext_size;
    size = (extents + 1) * ext_size;  // round

    string curr_date = utils::Chrono::DateString();
    std::lock_guard<std::mutex> guard(lock_);

    string pool_id = MPOOLIDS[type];
    char idbuf[64] = "";
    if (ess_.empty()) {
        LOG(ERROR) << "CreateBlob, extent server empty";
        return common::CYPRE_EM_POOL_NOT_READY;
    }
    auto iter = users_.find(user_id);
    if (iter == users_.end()) {
        LOG(ERROR) << "Not find user, id: " << user_id;
        return common::CYPRE_EM_USER_NOT_FOUND;
    }
    UserData &udata = iter->second;
    snprintf(idbuf, sizeof(idbuf) - 1, "blob%lu", next_blob_id_);
    next_blob_id_++;
    blob_id = idbuf;
    BlobData blob;
    blob.id = idbuf;
    blob.user_id = user_id;
    blob.pool_id = pool_id;
    blob.type = type;
    blob.size = size;
    blob.name = name;
    blob.desc = desc;
    blob.instance_id = instance_id;
    blob.create_date = curr_date;
    blob.update_date = curr_date;
    blobs_[blob.id] = blob;

    // update user pool
    if (std::find(udata.pool_ids.begin(), udata.pool_ids.end(), pool_id)
        == udata.pool_ids.end()) {
        udata.pool_ids.push_back(pool_id);
    }
    // set rg ids, and insert to router map
    auto iter2 = ess_.begin();
    for (size_t i = 1; i <= extents + 1; i++) {
        const string &rgid = iter2->first;
        ExtentRouterData er;
        er.extent_id = common::ExtentIDGenerator::GenerateExtentID(blob_id, i);
        er.blob_id = blob_id;
        er.rg_id = rgid;
        er.pool_id = pool_id;
        extent_routers_[er.extent_id] = er;
        if (++iter2 == ess_.end()) {
            iter2 = ess_.begin();
        }
    }
    LOG(INFO) << "Create blob succeed, user_id:" << user_id
              << ", blob_name:" << name << ", blob_id:" << idbuf
              << ", pool_id:" << pool_id;
    return common::CYPRE_OK;
}

int MockLogicManager::DeleteBlob(
        const std::string &user_id, const std::string &blob_id) {
    std::lock_guard<std::mutex> guard(lock_);
    auto iter = blobs_.find(blob_id);
    if (iter == blobs_.end()) {
        LOG(ERROR) << "Not find blob, blob id: " << blob_id;
        return common::CYPRE_OK;
    }
    if (iter->second.user_id != user_id) {
        LOG(ERROR) << "Have no permisson to delete blob"
                   << ", expected user: " << iter->second.user_id
                   << ", actual user: " << user_id;
        return common::CYPRE_ER_NO_PERMISSION;
    }
    blobs_.erase(iter);
    LOG(WARNING) << "delete blob ok, blob id: " << blob_id;
    return common::CYPRE_OK;
}

int MockLogicManager::RenameBlob(
        const std::string &user_id, const std::string &blob_id,
        const std::string &new_name) {
    std::lock_guard<std::mutex> guard(lock_);
    auto iter = blobs_.find(blob_id);
    if (iter == blobs_.end()) {
        LOG(ERROR) << "Not find blob, blob id: " << blob_id;
        return common::CYPRE_EM_BLOB_NOT_FOUND;
    }
    if (iter->second.user_id != user_id) {
        LOG(ERROR) << "Have no permisson to rename blob"
                   << ", expected user: " << iter->second.user_id
                   << ", actual user: " << user_id;
        return common::CYPRE_ER_NO_PERMISSION;
    }
    string curr_date = utils::Chrono::DateString();
    iter->second.name = new_name;
    iter->second.update_date = curr_date;
    LOG(WARNING) << "rename blob ok, blob id: " << blob_id;
    return common::CYPRE_OK;
}

int MockLogicManager::QueryBlob(
        const std::string &blob_id, bool all, BlobData *blob) {
    std::lock_guard<std::mutex> guard(lock_);
    auto iter = blobs_.find(blob_id);
    if (iter == blobs_.end()) {
        LOG(ERROR) << "Not find blob, blob id: " << blob_id;
        return common::CYPRE_EM_BLOB_NOT_FOUND;
    }
    *blob = iter->second;
    return common::CYPRE_OK;
}

int MockLogicManager::ListBlobs(
        const std::string &user_id, std::vector<BlobData> &llblobs) {
    std::lock_guard<std::mutex> guard(lock_);
    for (auto iter = blobs_.begin(); iter != blobs_.end(); ++iter) {
        if (iter->second.user_id != user_id) {
            continue;
        }
        llblobs.push_back(iter->second);
    }
    return common::CYPRE_OK;
}

int MockLogicManager::QueryRouter(
        const std::string &extent_id, std::string &rg_id, ESData &es) {
    std::lock_guard<std::mutex> guard(lock_);
    auto itr = extent_routers_.find(extent_id);
    if (itr == extent_routers_.end()) {
        LOG(ERROR) << " Query router failed, not found, extent id:"
                   << extent_id;
        return common::CYPRE_EM_ROUTER_NOT_FOUND;
    }
    const ExtentRouterData &rdata = itr->second;
    rg_id = rdata.rg_id;
    auto itr2 = ess_.find(rg_id);
    if (itr2 == ess_.end()) {
        LOG(ERROR) << " Query router failed, rg not found, extent id:"
                   << extent_id << ", rg_id:" << rg_id;
        return common::CYPRE_EM_RG_NOT_FOUND;
    }
    es = itr2->second;
    return common::CYPRE_OK;
}

int MockLogicManager::ResizeBlob() {
    return common::CYPRE_OK;
}

////// Class MockUserLogicImpl
void MockUserLogicImpl::CreateUser(
        const extentmanager::pb::CreateUserRequest *request,
        extentmanager::pb::CreateUserResponse *response) {
    if (request->user_name().empty() || request->email().empty()) {
        LOG(ERROR) << "Create user failed, username or email empty"
                   << ", username:" << request->user_name()
                   << ", email:" << request->email();
        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message("username or email empty");
        return;
    }
    string curr_date = utils::Chrono::DateString();
    UserData udata;
    udata.name = request->user_name();
    udata.email = request->email();
    udata.comments = request->comments();
    udata.create_date = curr_date;
    udata.update_date = curr_date;
    udata.set_id = SETID;
    string user_id;
    int rv = mgr_->CreateUser(udata, user_id);
    response->mutable_status()->set_code(rv);
    response->set_user_id(user_id);
}

void MockUserLogicImpl::DeleteUser(
        const extentmanager::pb::DeleteUserRequest *request,
        extentmanager::pb::DeleteUserResponse *response) {
    if (request->user_id().empty()) {
        LOG(ERROR) << "Delete user failed, userid empty";
        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message("userid empty");
        return;
    }
    const std::string &user_id = request->user_id();
    int rv = mgr_->DeleteUser(user_id);
    response->mutable_status()->set_code(rv);
    if (rv != common::CYPRE_OK) {
        response->mutable_status()->set_message("userid not found");
    }
}

void MockUserLogicImpl::UpdateUser(
        const extentmanager::pb::UpdateUserRequest *request,
        extentmanager::pb::UpdateUserResponse *response) {
    if (request->user_id().empty()) {
        LOG(ERROR) << "Update user failed, userid empty";
        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message("userid empty");
        return;
    }
    const std::string &user_id = request->user_id();
    int rv = mgr_->UpdateUser(
            user_id, request->new_name(), request->new_email(),
            request->new_comments());
    response->mutable_status()->set_code(rv);
    if (rv != common::CYPRE_OK) {
        response->mutable_status()->set_message("userid not found");
    }
}

void MockUserLogicImpl::QueryUser(
        const extentmanager::pb::QueryUserRequest *request,
        extentmanager::pb::QueryUserResponse *response) {
    if (request->user_id().empty()) {
        LOG(ERROR) << "Update user failed, userid empty";
        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message("userid empty");
        return;
    }
    const std::string &user_id = request->user_id();
    UserData udata;
    int rv = mgr_->QueryUser(user_id, &udata);
    response->mutable_status()->set_code(rv);
    if (rv != common::CYPRE_OK) {
        response->mutable_status()->set_message("userid not found");
    } else {
        common::pb::User user;
        udata.ToPbUser(user);
        *response->mutable_user() = user;
    }
}

void MockUserLogicImpl::ListUsers(
        const extentmanager::pb::ListUsersRequest *request,
        extentmanager::pb::ListUsersResponse *response) {
    vector<UserData> llusers;
    mgr_->ListUsers(llusers);
    for (auto iter = llusers.begin(); iter != llusers.end(); ++iter) {
        const UserData &udata = *iter;
        common::pb::User *user = response->add_users();
        udata.ToPbUser(*user);
    }
    response->mutable_status()->set_code(common::CYPRE_OK);
    LOG(INFO) << "List users succeed";
}

////// Class MockBlobLogicImpl
void MockBlobLogicImpl::CreateBlob(
        const extentmanager::pb::CreateBlobRequest *request,
        extentmanager::pb::CreateBlobResponse *response) {
    if (request->user_id().empty() || request->blob_name().empty()
        || request->instance_id().empty()
        || !utils::blobtype_valid(request->blob_type())) {
        LOG(ERROR) << "Create blob failed, argument invalid"
                   << ", user_id:" << request->user_id()
                   << ", blob_name:" << request->blob_name()
                   << ", instance_id:" << request->instance_id()
                   << ", blob_size:" << request->blob_size()
                   << ", blob_type:" << static_cast<int>(request->blob_type());
        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message("argument invalid");
        return;
    }
    uint64_t size = request->blob_size();
    extentmanager::BlobType blob_type =
            utils::FromPbBlobType(request->blob_type());
    extentmanager::PoolType pool_type;
    switch (blob_type) {
        case extentmanager::BlobType::kBlobTypeGeneral:
            pool_type = extentmanager::PoolType::kPoolTypeHDD;
        case extentmanager::BlobType::kBlobTypeSuperior:
            pool_type = extentmanager::PoolType::kPoolTypeSSD;
        case extentmanager::BlobType::kBlobTypeExcellent:
            pool_type = extentmanager::PoolType::kPoolTypeNVMeSSD;
        default:
            pool_type = extentmanager::PoolType::kPoolTypeHDD;
    }
    const std::string &user_id = request->user_id();
    const std::string &name = request->blob_name();
    const std::string &instance_id = request->instance_id();
    const std::string &desc = request->blob_desc();

    std::string blob_id;
    int rv = mgr_->CreateBlob(
            user_id, pool_type, name, size, blob_type, desc, instance_id,
            blob_id);
    response->mutable_status()->set_code(rv);
    if (rv != common::CYPRE_OK) {
        response->mutable_status()->set_message("create failed");
    } else {
        response->set_blob_id(blob_id);
    }
}

void MockBlobLogicImpl::DeleteBlob(
        const extentmanager::pb::DeleteBlobRequest *request,
        extentmanager::pb::DeleteBlobResponse *response) {
    // 1. check param
    if (request->user_id().empty() || request->blob_id().empty()) {
        LOG(ERROR) << "Delete blob failed, userid or blobid empty"
                   << ", user_id: " << request->user_id()
                   << ", blob_id: " << request->blob_id();
        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message("userid or blobid empty");
        return;
    }
    int rv = mgr_->DeleteBlob(request->user_id(), request->blob_id());
    response->mutable_status()->set_code(rv);
    if (rv != common::CYPRE_OK) {
        response->mutable_status()->set_message("delete failed");
    }
}

void MockBlobLogicImpl::RenameBlob(
        const extentmanager::pb::RenameBlobRequest *request,
        extentmanager::pb::RenameBlobResponse *response) {
    // 1. check param
    if (request->user_id().empty() || request->blob_id().empty()
        || request->new_name().empty()) {
        LOG(ERROR) << "Rename blob failed, userid or blobid or newname empty"
                   << ", user_id:" << request->user_id()
                   << ", blob_id:" << request->blob_id()
                   << ", new_name:" << request->new_name();
        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message(
                "userid or blobid or newname empty");
        return;
    }
    int rv = mgr_->RenameBlob(
            request->user_id(), request->blob_id(), request->new_name());
    response->mutable_status()->set_code(rv);
    if (rv != common::CYPRE_OK) {
        response->mutable_status()->set_message("rename failed");
    }
}

void MockBlobLogicImpl::QueryBlob(
        const extentmanager::pb::QueryBlobRequest *request,
        extentmanager::pb::QueryBlobResponse *response) {
    // 1. check param
    if (request->blob_id().empty()) {
        LOG(ERROR) << "Query blob failed, blobid empty, blob_id:"
                   << request->blob_id();
        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message("blob_id empty");
        return;
    }

    // 2. query blob
    BlobData blob;
    int rv = mgr_->QueryBlob(request->blob_id(), false, &blob);
    response->mutable_status()->set_code(rv);
    if (rv != common::CYPRE_OK) {
        response->mutable_status()->set_message("query failed");
    } else {
        uint64_t ext_size = GlobalConfig::Instance()->GetExtentSize();
        common::pb::Blob pb_blob;
        blob.ToPbBlob(pb_blob);
        *(response->mutable_blob()) = pb_blob;
        response->set_extent_size(ext_size);
        response->set_max_block_size(MAX_BLOCK_SIZE);
    }
}

void MockBlobLogicImpl::ListBlobs(
        const extentmanager::pb::ListBlobsRequest *request,
        extentmanager::pb::ListBlobsResponse *response) {
    // 1. check param
    if (request->user_id().empty()) {
        LOG(ERROR) << "List blob failed, userid empty";
        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message("userid empty");
        return;
    }
    std::vector<BlobData> llblobs;
    int rv = mgr_->ListBlobs(request->user_id(), llblobs);
    response->mutable_status()->set_code(rv);
    if (rv != common::CYPRE_OK) {
        response->mutable_status()->set_message("list failed");
    } else {
        for (auto iter = llblobs.begin(); iter != llblobs.end(); ++iter) {
            common::pb::Blob *pb_blob = response->add_blobs();
            iter->ToPbBlob(*pb_blob);
        }
    }
}

void MockBlobLogicImpl::ResizeBlob(
        const extentmanager::pb::ResizeBlobRequest *request,
        extentmanager::pb::ResizeBlobResponse *response) {
    response->mutable_status()->set_code(common::CYPRE_ER_NOT_SUPPORTED);
}

void MockBlobLogicImpl::QueryRouter(
        const extentmanager::pb::QueryRouterRequest *request,
        extentmanager::pb::QueryRouterResponse *response) {
    if (request->extent_id().empty()) {
        LOG(ERROR) << "Query router failed, extent id empty";
        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message("extentid empty");
        return;
    }
    std::string rg_id;
    ESData es;
    int rv = mgr_->QueryRouter(request->extent_id(), rg_id, es);
    response->mutable_status()->set_code(rv);
    if (rv != common::CYPRE_OK) {
        response->mutable_status()->set_message("list failed");
    } else {
        common::pb::ExtentRouter router;
        router.set_extent_id(request->extent_id());
        router.set_rg_id(rg_id);
        router.mutable_primary()->set_es_id(es.id);
        router.mutable_primary()->set_public_ip(es.ip);
        router.mutable_primary()->set_public_port(es.port);
        router.mutable_primary()->set_private_ip(es.ip);
        router.mutable_primary()->set_private_port(es.port);
        response->set_router_version(1);
        *response->mutable_router() = router;
    }
}

////// Class MockeExtentIoLogicImpl
void MockExtentIoLogicImpl::Read(
        google::protobuf::RpcController *cntl,
        const extentserver::pb::ReadRequest *request,
        extentserver::pb::ReadResponse *response) {
    if (request->extent_id().empty()) {
        LOG(ERROR) << "Read failed, extent id empty";
        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message("extentid empty");
        return;
    }
    butil::IOBuf iobuf;
    brpc::Controller *bcntl = static_cast<brpc::Controller *>(cntl);
    int rv = exmgr_->Read(
            request->extent_id(), request->offset(), request->size(), iobuf, response);
    response->mutable_status()->set_code(rv);
    if (rv != common::CYPRE_OK) {
        response->mutable_status()->set_message("read failed");
    } else {
        bcntl->response_attachment() = iobuf;
    }
}

void MockExtentIoLogicImpl::Write(
        google::protobuf::RpcController *cntl,
        const extentserver::pb::WriteRequest *request,
        extentserver::pb::WriteResponse *response) {

    if (request->extent_id().empty()) {
        LOG(ERROR) << "Write failed, extent id empty";
        response->mutable_status()->set_code(common::CYPRE_ER_INVALID_ARGUMENT);
        response->mutable_status()->set_message("extentid empty");
        return;
    }
    brpc::Controller *bcntl = static_cast<brpc::Controller *>(cntl);
    const butil::IOBuf &iobuf = bcntl->request_attachment();
    int rv = exmgr_->Write(
            request->extent_id(), request->offset(), request->size(), iobuf);
    response->mutable_status()->set_code(rv);
    if (rv != common::CYPRE_OK) {
        response->mutable_status()->set_message("write failed");
    }
}

///////////////////Mocked Entent Io logic
// do nothing extent io
int MockEmptyExtentManager::Read(
        const std::string &extent_id, uint64_t offset, uint64_t len,
        butil::IOBuf &iobuf, extentserver::pb::ReadResponse *response) {
    iobuf.resize(len);
    return common::CYPRE_OK;
}

int MockEmptyExtentManager::Write(
        const std::string &extent_id, uint64_t offset, uint64_t len,
        const butil::IOBuf &iobuf) {
    return common::CYPRE_OK;
}

// memory extent io
int MockMemExtentManager::Read(
        const std::string &extent_id, uint64_t offset, uint64_t len,
        butil::IOBuf &iobuf, extentserver::pb::ReadResponse *response) {
    uint64_t ext_size = GlobalConfig::Instance()->GetExtentSize();
    if (offset >= ext_size || offset + len > ext_size) {
        LOG(ERROR) << "Read failed, offset or len invalid";
        return common::CYPRE_ER_INVALID_ARGUMENT;
    }

    std::lock_guard<std::mutex> guard(lock_);
    auto itr = extents_.find(extent_id);
    if (itr == extents_.end()) {
        char *ebuf = new char[len];
        memset(ebuf, '\0', len);
        iobuf.append(ebuf, len);
        response->set_crc32(utils::Crc32::Checksum(ebuf, len));
        delete ebuf;
        return common::CYPRE_OK;
    } else {
        const char *extent = itr->second;
        int ret = iobuf.append(extent + offset, len);
        if (ret != 0) {
            LOG(ERROR) << "Read failed, append IOBuf failed:" << ret;
            return common::CYPRE_C_INTERNAL_ERROR;
        }
        response->set_crc32(utils::Crc32::Checksum(extent + offset, len));
    }
    return common::CYPRE_OK;
}

int MockMemExtentManager::Write(
        const std::string &extent_id, uint64_t offset, uint64_t len,
        const butil::IOBuf &iobuf) {
    const uint64_t ext_size = GlobalConfig::Instance()->GetExtentSize();
    if (offset >= ext_size || offset + len > ext_size) {
        LOG(ERROR) << "Write failed, offset or len invalid";
        return common::CYPRE_ER_INVALID_ARGUMENT;
    }

    std::lock_guard<std::mutex> guard(lock_);
    auto itr = extents_.find(extent_id);
    if (itr == extents_.end()) {
        char *extent = new char[ext_size];
        memset(extent, 0, ext_size);
        extents_[extent_id] = extent;
        itr = extents_.find(extent_id);
    }
    char *extent = itr->second;
    iobuf.copy_to(extent + offset, len, 0);
    return common::CYPRE_OK;
}
