/*
 * Copyright 2020 JDD authors.
 * @feifei5
 *
 */

#include "router_bench.h"

#include <iostream>
#include <thread>

namespace cyprestore {
namespace tools {

uint64_t blob_size =
        1 * (1ULL << 40);  // 1TB, for 1GB extentsize, there are 1024 extents

int RouterBench::BenchCreate() {
    int blob_each_thread = options_.blob_num / options_.thread_num;
    std::cout << blob_each_thread << " blobs each thread." << std::endl;

    time_t start = getTimeStamp();
    std::vector<std::thread> threads;
    for (int i = 0; i < options_.thread_num; i++) {
        threads.push_back(
                std::thread(&RouterBench::CreateBlob, this, blob_each_thread));
    }
    for (auto &th : threads) {
        th.join();
    }
    time_t end = getTimeStamp();
    std::cout << "Create " << options_.blob_num << " blob cost " << end - start
              << " us" << std::endl;
    return 0;
}

int RouterBench::BenchQuery() {
    // first create blob
    if (PreCreateBlob() != 0) {
        std::cerr << "Create blob failed." << std::endl;
    }

    // first query in store
    int blob_each_thread = options_.blob_num / options_.thread_num;
    int begin = 0;
    std::vector<std::thread> threads;
    time_t start = getTimeStamp();
    for (int i = 0; i < options_.thread_num; i++) {
        std::vector<std::string> tmp;
        for (int j = i * blob_each_thread; j < (i + 1) * blob_each_thread;
             j++) {
            tmp.push_back(blob_ids_[j]);
        }
        threads.push_back(std::thread(&RouterBench::Query, this, tmp));
        begin += blob_each_thread;
    }

    for (auto &th : threads) {
        th.join();
    }
    time_t end = getTimeStamp();
    int total_blob = options_.blob_num;
    std::cout << "Query " << total_blob << " blob cost " << end - start << " us"
              << std::endl;

    // second query in cache
    threads.clear();
    start = getTimeStamp();
    for (int i = 0; i < options_.thread_num; i++) {
        std::vector<std::string> tmp;
        for (int j = i * blob_each_thread; j < (i + 1) * blob_each_thread;
             j++) {
            tmp.push_back(blob_ids_[j]);
        }
        threads.push_back(std::thread(&RouterBench::Query, this, tmp));
        begin += blob_each_thread;
    }

    for (auto &th : threads) {
        th.join();
    }
    end = getTimeStamp();
    total_blob = options_.blob_num;
    std::cout << "Query " << total_blob << " blob cost " << end - start << " us"
              << std::endl;
    return 0;
}

void RouterBench::Query(std::vector<std::string> blob_ids) {
    int num_each_blob = blob_size / (1024 * 1024 * 1024);
    int total_cost = 0;
    for (auto &blob : blob_ids) {
        for (int i = 1; i <= num_each_blob; i++) {
            brpc::Controller cntl;
            extentmanager::pb::QueryRouterRequest request;
            extentmanager::pb::QueryRouterResponse response;

            std::string extent_id = blob + "." + std::to_string(i);
            request.set_extent_id(extent_id);
            time_t begin = getTimeStamp();
            router_stub_->QueryRouter(&cntl, &request, &response, NULL);
            time_t end = getTimeStamp();
            if (cntl.Failed()) {
                std::cerr << "Failed to query router, err: "
                          << response.status().message() << std::endl;
                return;
            } else if (response.status().code() != 0) {
                std::cerr << "Send request failed, err:" << cntl.ErrorText()
                          << std::endl;
                return;
            }
            total_cost += end - begin;
        }
    }
    int total_extent = blob_ids.size() * num_each_blob;
    int cost = total_cost / total_extent;
    std::cout << "Query " << total_extent << " extent each cost " << cost
              << " us" << std::endl;
}

int RouterBench::CreateBlob(int num) {
    time_t begin = getTimeStamp();
    for (int i = 0; i < num; i++) {
        brpc::Controller cntl;
        extentmanager::pb::CreateBlobRequest request;
        extentmanager::pb::CreateBlobResponse response;

        request.set_user_id(options_.user_id);
        request.set_blob_name("bench");
        request.set_blob_type(common::pb::BlobType::BLOB_TYPE_SUPERIOR);
        request.set_blob_size(blob_size);
        request.set_instance_id("1");
        request.set_pool_id("pool-a");
        blob_stub_->CreateBlob(&cntl, &request, &response, NULL);
        if (cntl.Failed()) {
            std::cerr << "Failed to create blob, err: "
                      << response.status().message() << std::endl;
            return -1;
        } else if (response.status().code() != 0) {
            std::cerr << "Send request failed, err:" << cntl.ErrorText()
                      << std::endl;
            return -1;
        }
        // blob_ids_.push_back(response.blob_id());
    }
    time_t end = getTimeStamp();
    std::cout << "Create blob cost " << end - begin << " us" << std::endl;
    return 0;
}

int RouterBench::PreCreateBlob() {
    time_t begin = getTimeStamp();
    for (int i = 0; i < options_.blob_num; i++) {
        brpc::Controller cntl;
        extentmanager::pb::CreateBlobRequest request;
        extentmanager::pb::CreateBlobResponse response;

        request.set_user_id(options_.user_id);
        request.set_blob_name("bench");
        request.set_blob_type(common::pb::BlobType::BLOB_TYPE_SUPERIOR);
        request.set_blob_size(blob_size);
        request.set_instance_id("1");
        request.set_pool_id("pool-a");
        blob_stub_->CreateBlob(&cntl, &request, &response, NULL);
        if (cntl.Failed()) {
            std::cerr << "Failed to create blob, err: "
                      << response.status().message() << std::endl;
            return -1;
        } else if (response.status().code() != 0) {
            std::cerr << "Send request failed, err:" << cntl.ErrorText()
                      << std::endl;
            return -1;
        }
        blob_ids_.push_back(response.blob_id());
    }
    time_t end = getTimeStamp();
    std::cout << "Create blob cost " << end - begin << " us" << std::endl;
    return 0;
}

int RouterBench::init() {
    brpc::ChannelOptions channel_options;
    channel_options.protocol = options_.protocal;
    channel_options.connection_type = options_.connection_type;
    channel_options.timeout_ms = options_.timeout_ms;
    channel_options.max_retry = options_.max_retry;

    // for router query
    if (router_channel_.Init(
                options_.ExtentManagerAddr().c_str(), &channel_options)
        != 0) {
        std::cerr << "Failed to initialize router channel" << std::endl;
        return -1;
    }

    router_stub_ = new extentmanager::pb::RouterService_Stub(&router_channel_);
    if (!router_stub_) {
        std::cerr << "Failed to new access service stub" << std::endl;
        return -1;
    }

    // for blob create
    if (blob_channel_.Init(
                options_.ExtentManagerAddr().c_str(), &channel_options)
        != 0) {
        std::cerr << "Failed to initialize blob channel" << std::endl;
        return -1;
    }

    blob_stub_ = new extentmanager::pb::ResourceService_Stub(&blob_channel_);
    if (!blob_stub_) {
        std::cerr << "Failed to new access service stub" << std::endl;
        return -1;
    }

    initialized_ = true;
    return 0;
}

std::time_t RouterBench::getTimeStamp() {
    std::chrono::time_point<
            std::chrono::system_clock, std::chrono::microseconds>
            tp = std::chrono::time_point_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now());  //获取当前时间点
    std::time_t timestamp =
            tp.time_since_epoch().count();  //计算距离1970-1-1,00:00的时间长度
    return timestamp;
}

int RouterBench::Exec() {
    if (!initialized_ && init() != 0) {
        return -1;
    }
    switch (options_.cmd) {
        case Cmd::kBenchCreate:
            return BenchCreate();
        case Cmd::kBenchQuery:
            return BenchQuery();
        default:
            return -1;
    }
}

}  // namespace tools
}  // namespace cyprestore
