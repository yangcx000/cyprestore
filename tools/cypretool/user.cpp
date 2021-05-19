/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "user.h"

#include <iostream>

namespace cyprestore {
namespace tools {

common::pb::PoolType User::GetPoolType() {
    if (options_.pool_type == "general") {
        return common::pb::PoolType::POOL_TYPE_HDD;
    } else if (options_.pool_type == "superior") {
        return common::pb::PoolType::POOL_TYPE_SSD;
    } else if (options_.pool_type == "excellent") {
        return common::pb::PoolType::POOL_TYPE_NVME_SSD;
    }
    return common::pb::PoolType::POOL_TYPE_UNKNOWN;
}

int User::Create() {
    brpc::Controller cntl;
    access::pb::CreateUserRequest request;
    access::pb::CreateUserResponse response;

    request.set_user_name(options_.user_name);
    if (!options_.email.empty()) request.set_email(options_.email);

    if (!options_.comments.empty()) request.set_comments(options_.comments);

    if (!options_.pool_type.empty()) request.set_pool_type(GetPoolType());

    stub_->CreateUser(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        std::cerr << "Send request failed, err:" << cntl.ErrorText()
                  << std::endl;
        return -1;
    } else if (response.status().code() != 0) {
        std::cerr << "Failed to create user, err: "
                  << response.status().message() << std::endl;
        return -1;
    }

    std::cout << "Create user " << options_.user_name << " succeed"
              << ", user_id:" << response.user_id() << std::endl;
    return 0;
}

int User::Delete() {
    brpc::Controller cntl;
    access::pb::DeleteUserRequest request;
    access::pb::DeleteUserResponse response;

    request.set_user_id(options_.user_id);
    stub_->DeleteUser(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        std::cerr << "Send request failed, err:" << cntl.ErrorText()
                  << std::endl;
        return -1;
    } else if (response.status().code() != 0) {
        std::cerr << "Failed to delete user, err: "
                  << response.status().message() << std::endl;
        return -1;
    }

    std::cout << "Delete user " << options_.user_id << " succeed" << std::endl;
    return 0;
}

int User::Query() {
    brpc::Controller cntl;
    access::pb::QueryUserRequest request;
    access::pb::QueryUserResponse response;

    request.set_user_id(options_.user_id);
    stub_->QueryUser(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        std::cerr << "Send request failed, err:" << cntl.ErrorText()
                  << std::endl;
        return -1;
    } else if (response.status().code() != 0) {
        std::cerr << "Failed to query user, err: "
                  << response.status().message() << std::endl;
        return -1;
    }

    Describe(response.userinfo());
    return 0;
}

int User::List() {
    brpc::Controller cntl;
    access::pb::ListUsersRequest request;
    access::pb::ListUsersResponse response;

    request.set_set_id(options_.set_id);
    stub_->ListUsers(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        std::cerr << "Send request failed, err:" << cntl.ErrorText()
                  << std::endl;
        return -1;
    } else if (response.status().code() != 0) {
        std::cerr << "Failed to list user, err: " << response.status().message()
                  << std::endl;
        return -1;
    }

    std::cout << "There are " << response.users_size()
              << " users:" << std::endl;
    for (int i = 0; i < response.users_size(); i++) {
        Describe(response.users(i));
    }
    return 0;
}

int User::Update() {
    brpc::Controller cntl;
    access::pb::UpdateUserRequest request;
    access::pb::UpdateUserResponse response;

    request.set_user_id(options_.user_id);
    request.set_new_name(options_.user_new_name);
    request.set_new_email(options_.new_email);
    request.set_new_comments(options_.new_comments);
    stub_->UpdateUser(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        std::cerr << "Send request failed, err:" << cntl.ErrorText()
                  << std::endl;
        return -1;
    } else if (response.status().code() != 0) {
        std::cerr << "Failed to update user, err: "
                  << response.status().message() << std::endl;
        return -1;
    }

    std::cout << "Update user succeed" << std::endl;
    return 0;
}

void User::Describe(const common::pb::User &user) {
    std::cout << "*********************Display user*********************"
              << "\n\t id:" << user.id() << "\n\t name:" << user.name()
              << "\n\t email:" << user.email()
              << "\n\t comments:" << user.comments()
              << "\n\t pool_id:" << user.pool_id()
              << "\n\t set_id:" << user.set_id()
              << "\n\t create_time:" << user.create_date()
              << "\n\t update_time:" << user.update_date() << std::endl;
}

int User::init() {
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

int User::Exec() {
    if (!initialized_ && init() != 0) {
        return -1;
    }

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
        case Cmd::kUpdate:
            ret = Update();
            break;
        default:
            break;
    }

    return ret;
}

}  // namespace tools
}  // namespace cyprestore
