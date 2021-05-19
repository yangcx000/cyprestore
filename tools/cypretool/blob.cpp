/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "blob.h"

#include <iostream>

namespace cyprestore {
namespace tools {

static const std::string GENERAL = "general";
static const std::string SUPERIOR = "superior";
static const std::string EXCELLENT = "excellent";

common::pb::BlobType Blob::getBlobType() {
    if (options_.blob_type == GENERAL) {
        return common::pb::BlobType::BLOB_TYPE_GENERAL;
    } else if (options_.blob_type == SUPERIOR) {
        return common::pb::BlobType::BLOB_TYPE_SUPERIOR;
    } else if (options_.blob_type == EXCELLENT) {
        return common::pb::BlobType::BLOB_TYPE_EXCELLENT;
    }

    return common::pb::BlobType::BLOB_TYPE_UNKNOWN;
}

std::string Blob::getBlobType(common::pb::BlobType type) {
    if (type == common::pb::BlobType::BLOB_TYPE_GENERAL) {
        return GENERAL;
    } else if (type == common::pb::BlobType::BLOB_TYPE_SUPERIOR) {
        return SUPERIOR;
    } else if (type == common::pb::BlobType::BLOB_TYPE_EXCELLENT) {
        return EXCELLENT;
    }
    return "unknown";
}

void Blob::describe(const common::pb::Blob &blob) {
    std::cout << "*********************Blob Information*********************"
              << "\n\t blob id:" << blob.id()
              << "\n\t blob name:" << blob.name()
              << "\n\t blob size:" << blob.size()
              << "\n\t blob type:" << getBlobType(blob.type())
              << "\n\t blob desc:" << blob.desc()
              << "\n\t blob status:" << blob.status()
              << "\n\t blob pool id:" << blob.pool_id()
              << "\n\t blob user id:" << blob.user_id()
              << "\n\t blob inst id:" << blob.instance_id()
              << "\n\t blob create time:" << blob.create_date()
              << "\n\t blob update time:" << blob.update_date() << std::endl;
}

int Blob::Create() {
    brpc::Controller cntl;
    access::pb::CreateBlobRequest request;
    access::pb::CreateBlobResponse response;

    request.set_user_id(options_.user_id);
    request.set_blob_name(options_.blob_name);
    request.set_blob_type(getBlobType());
    request.set_blob_size(options_.blob_size);
    request.set_instance_id(options_.instance_id);

    if (!options_.pool_id.empty()) request.set_pool_id(options_.pool_id);

    if (!options_.blob_desc.empty()) request.set_blob_desc(options_.blob_desc);

    stub_->CreateBlob(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        std::cerr << "Send request failed, err:" << cntl.ErrorText()
                  << std::endl;
        return -1;
    } else if (response.status().code() != 0) {
        std::cerr << "Failed to create blob, err: "
                  << response.status().message() << std::endl;
        return -1;
    }

    std::cout << "Create blob " << options_.blob_name << " succeed"
              << ", blob_id:" << response.blob_id() << std::endl;
    return 0;
}

int Blob::Rename() {
    brpc::Controller cntl;
    access::pb::RenameBlobRequest request;
    access::pb::RenameBlobResponse response;

    request.set_user_id(options_.user_id);
    request.set_blob_id(options_.blob_id);
    request.set_new_name(options_.blob_new_name);

    stub_->RenameBlob(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        std::cerr << "Send request failed, err:" << cntl.ErrorText()
                  << std::endl;
        return -1;
    } else if (response.status().code() != 0) {
        std::cerr << "Failed to rename blob, err: "
                  << response.status().message() << std::endl;
        return -1;
    }

    std::cout << "Rename blob " << options_.blob_id << " succeed" << std::endl;
    return 0;
}

int Blob::Resize() {
    brpc::Controller cntl;
    access::pb::ResizeBlobRequest request;
    access::pb::ResizeBlobResponse response;

    request.set_user_id(options_.user_id);
    request.set_blob_id(options_.blob_id);
    request.set_new_size(options_.blob_new_size);

    stub_->ResizeBlob(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        std::cerr << "Send request failed, err:" << cntl.ErrorText()
                  << std::endl;
        return -1;
    } else if (response.status().code() != 0) {
        std::cerr << "Failed to resize blob, err: "
                  << response.status().message() << std::endl;
        return -1;
    }

    std::cout << "Rename blob " << options_.blob_id << " succeed" << std::endl;
    return 0;
}

int Blob::Delete() {
    brpc::Controller cntl;
    access::pb::DeleteBlobRequest request;
    access::pb::DeleteBlobResponse response;

    request.set_user_id(options_.user_id);
    request.set_blob_id(options_.blob_id);

    stub_->DeleteBlob(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        std::cerr << "Send request failed, err:" << cntl.ErrorText()
                  << std::endl;
        return -1;
    } else if (response.status().code() != 0) {
        std::cerr << "Failed to delete blob, err: "
                  << response.status().message() << std::endl;
        return -1;
    }

    std::cout << "Delete blob " << options_.blob_id << " succeed" << std::endl;
    return 0;
}

int Blob::Query() {
    brpc::Controller cntl;
    access::pb::QueryBlobRequest request;
    access::pb::QueryBlobResponse response;

    request.set_blob_id(options_.blob_id);
    request.set_user_id(options_.user_id);

    stub_->QueryBlob(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        std::cerr << "Send request failed, err:" << cntl.ErrorText()
                  << std::endl;
        return -1;
    } else if (response.status().code() != 0) {
        std::cerr << "Failed to query blob, err: "
                  << response.status().message() << std::endl;
        return -1;
    }

    std::cout << "Query blob " << options_.blob_id << " succeed" << std::endl;
    describe(response.blob_info());
    return 0;
}

int Blob::List() {
    brpc::Controller cntl;
    access::pb::ListBlobsRequest request;
    access::pb::ListBlobsResponse response;

    request.set_user_id(options_.user_id);

    stub_->ListBlobs(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        std::cerr << "Send request failed, err:" << cntl.ErrorText()
                  << std::endl;
        return -1;
    } else if (response.status().code() != 0) {
        std::cerr << "Failed to list blob, err: " << response.status().message()
                  << std::endl;
        return -1;
    }

    std::cout << "There are " << response.blobs_size()
              << " blobs:" << std::endl;
    for (int i = 0; i < response.blobs_size(); i++) {
        describe(response.blobs(i));
    }
    return 0;
}

int Blob::init() {
    brpc::ChannelOptions channel_options;
    channel_options.protocol = options_.protocal;
    channel_options.connection_type = options_.connection_type;
    channel_options.timeout_ms = options_.timeout_ms;
    channel_options.max_retry = options_.max_retry;

    if (channel_.Init(options_.AccessAddr().c_str(), &channel_options) != 0) {
        std::cerr << "Failed to initialize channel" << std::endl;
        return -1;
    }

    stub_ = new access::pb::AccessService_Stub(&channel_);
    if (!stub_) {
        std::cerr << "Failed to new access service stub" << std::endl;
        return -1;
    }

    initialized_ = true;
    return 0;
}

int Blob::Exec() {
    if (!initialized_ && init() != 0) return -1;

    int ret = -1;
    switch (options_.cmd) {
        case Cmd::kCreate:
            ret = Create();
            break;
        case Cmd::kDelete:
            ret = Delete();
            break;
        case Cmd::kQuery:
            ret = Query();
            break;
        case Cmd::kList:
            ret = List();
            break;
        case Cmd::kRename:
            ret = Rename();
            break;
        case Cmd::kResize:
            ret = Resize();
            break;
        default:
            break;
    }

    return ret;
}

}  // namespace tools
}  // namespace cyprestore
