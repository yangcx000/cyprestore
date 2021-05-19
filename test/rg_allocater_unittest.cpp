/*
 * Copyright 2020 JDD authors.
 * @feifei5
 *
 */

#include "extentmanager/rg_allocater.h"

#include <map>

#include "extentmanager/extent_server.h"
#include "extentmanager/rg_allocater_by_es.h"
#include "extentmanager/rg_allocater_by_host.h"
#include "extentmanager/rg_allocater_by_rack.h"
#include "gtest/gtest.h"
#include "utils/chrono.h"

namespace cyprestore {
namespace extentmanager {
namespace {

class RGAllocaterTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto es1 = std::make_shared<ExtentServer>(
                1, "es-1", ESStatus::kESStatusOk, "127.0.0.1", 8080,
                "127.0.0.1", 8081, 0, 1099511627776, "pool-1", "host1",
                "rack1");
        auto es2 = std::make_shared<ExtentServer>(
                2, "es-2", ESStatus::kESStatusOk, "127.0.0.2", 8080,
                "127.0.0.2", 8081, 0, 1099511627776, "pool-1", "host1",
                "rack1");
        auto es3 = std::make_shared<ExtentServer>(
                3, "es-3", ESStatus::kESStatusOk, "127.0.0.3", 8080,
                "127.0.0.3", 8081, 0, 1099511627776, "pool-1", "host2",
                "rack2");
        auto es4 = std::make_shared<ExtentServer>(
                4, "es-4", ESStatus::kESStatusOk, "127.0.0.4", 8080,
                "127.0.0.4", 8081, 0, 1099511627776, "pool-1", "host2",
                "rack2");
        auto es5 = std::make_shared<ExtentServer>(
                5, "es-5", ESStatus::kESStatusOk, "127.0.0.5", 8080,
                "127.0.0.5", 8081, 0, 1099511627776, "pool-1", "host3",
                "rack3");
        auto es6 = std::make_shared<ExtentServer>(
                6, "es-6", ESStatus::kESStatusOk, "127.0.0.6", 8080,
                "127.0.0.6", 8081, 0, 1099511627776, "pool-1", "host3",
                "rack3");
        ess_.push_back(es1);
        ess_.push_back(es2);
        ess_.push_back(es3);
        ess_.push_back(es4);
        ess_.push_back(es5);
        ess_.push_back(es6);

        auto es11 = std::make_shared<ExtentServer>(
                11, "es-11", ESStatus::kESStatusOk, "127.0.0.11", 8080,
                "127.0.0.11", 8081, 0, 2199023255552, "pool-1", "host1",
                "rack1");
        auto es12 = std::make_shared<ExtentServer>(
                12, "es-12", ESStatus::kESStatusOk, "127.0.0.12", 8080,
                "127.0.0.12", 8081, 0, 1099511627776, "pool-1", "host1",
                "rack1");
        auto es13 = std::make_shared<ExtentServer>(
                13, "es-13", ESStatus::kESStatusOk, "127.0.0.13", 8080,
                "127.0.0.13", 8081, 0, 2199023255552, "pool-1", "host2",
                "rack2");
        auto es14 = std::make_shared<ExtentServer>(
                14, "es-14", ESStatus::kESStatusOk, "127.0.0.14", 8080,
                "127.0.0.14", 8081, 0, 1099511627776, "pool-1", "host3",
                "rack2");
        auto es15 = std::make_shared<ExtentServer>(
                15, "es-15", ESStatus::kESStatusOk, "127.0.0.15", 8080,
                "127.0.0.15", 8081, 0, 1099511627776, "pool-1", "host4",
                "rack3");
        auto es16 = std::make_shared<ExtentServer>(
                16, "es-16", ESStatus::kESStatusOk, "127.0.0.16", 8080,
                "127.0.0.16", 8081, 0, 2199023255552, "pool-1", "host5",
                "rack3");
        ess1_.push_back(es11);
        ess1_.push_back(es12);
        ess1_.push_back(es13);
        ess1_.push_back(es14);
        ess1_.push_back(es15);
        ess1_.push_back(es16);
    }

    void TearDown() override {}

    std::shared_ptr<RGAllocater> rg_allocater_;
    std::vector<std::shared_ptr<ExtentServer>> ess_;
    std::vector<std::shared_ptr<ExtentServer>> ess1_;

    int rg_num_ = 900;
    int replica_num_ = 3;
    int rack_num_ = 3;
    int host_num_ = 4;
    int es_num_ = 6;
    // rg_id -> vector<es_id>
    std::unordered_map<int, std::vector<int>> rg_es_map_;
};

TEST_F(RGAllocaterTest, TestAllocaterByEs) {
    // same weight
    rg_allocater_ = std::shared_ptr<RGAllocaterByEs>(new RGAllocaterByEs());
    rg_allocater_->init(ess_, 1073741824);
    for (int i = 0; i < rg_num_; i++) {
        auto es = rg_allocater_->select_primary();
        rg_es_map_[i].push_back(es->get_id());
    }

    for (int i = 1; i < replica_num_; i++) {
        for (int j = 0; j < rg_num_; j++) {
            auto es = rg_allocater_->select_secondary(rg_es_map_[j]);
            rg_es_map_[j].push_back(es->get_id());
        }
    }

    for (auto &es : ess_) {
        ASSERT_EQ(rg_num_ * replica_num_ / es_num_, es->get_rgs());
        ASSERT_EQ(rg_num_ / es_num_, es->get_pri_rgs());
        ASSERT_EQ(rg_num_ * (replica_num_ - 1) / es_num_, es->get_sec_rgs());
    }
}

TEST_F(RGAllocaterTest, TestAllocaterByEsWithDifferWeight) {
    // different weight
    rg_allocater_ = std::shared_ptr<RGAllocaterByEs>(new RGAllocaterByEs());
    rg_allocater_->init(ess1_, 1073741824);
    for (int i = 0; i < rg_num_; i++) {
        auto es = rg_allocater_->select_primary();
        rg_es_map_[i].push_back(es->get_id());
    }

    for (int i = 1; i < replica_num_; i++) {
        for (int j = 0; j < rg_num_; j++) {
            auto es = rg_allocater_->select_secondary(rg_es_map_[j]);
            rg_es_map_[j].push_back(es->get_id());
        }
    }

    int total_weight = 0;
    for (auto &es : ess1_) {
        total_weight += es->get_weight();
    }

    for (auto &es : ess1_) {
        ASSERT_EQ(
                es->get_weight() * rg_num_ * replica_num_ / total_weight,
                es->get_rgs());
        ASSERT_EQ(es->get_weight() * rg_num_ / total_weight, es->get_pri_rgs());
        ASSERT_EQ(
                es->get_weight() * rg_num_ * (replica_num_ - 1) / total_weight,
                es->get_sec_rgs());
    }
}

TEST_F(RGAllocaterTest, TestAllocaterByHost) {
    rg_allocater_ = std::shared_ptr<RGAllocaterByHost>(new RGAllocaterByHost());
    rg_allocater_->init(ess_, 1073741824);
    for (int i = 0; i < rg_num_; i++) {
        auto es = rg_allocater_->select_primary();
        rg_es_map_[i].push_back(es->get_id());
    }

    for (int i = 1; i < replica_num_; i++) {
        for (int j = 0; j < rg_num_; j++) {
            auto es = rg_allocater_->select_secondary(rg_es_map_[j]);
            rg_es_map_[j].push_back(es->get_id());
        }
    }

    std::map<std::string, int> host_rgs_map;
    std::map<std::string, int> host_pri_rgs_map;
    std::map<std::string, int> host_sec_rgs_map;
    std::map<std::string, int> host_num_map;
    int total_weight = 0;
    for (auto &es : ess_) {
        host_rgs_map[es->get_host()] += es->get_rgs();
        host_pri_rgs_map[es->get_host()] += es->get_pri_rgs();
        host_sec_rgs_map[es->get_host()] += es->get_sec_rgs();
        host_num_map[es->get_host()] += es->get_weight();
        total_weight += es->get_weight();
    }

    for (auto &m : host_rgs_map) {
        ASSERT_EQ(
                host_num_map[m.first] * rg_num_ * replica_num_ / total_weight,
                m.second);
    }
    for (auto &m : host_pri_rgs_map) {
        ASSERT_EQ(host_num_map[m.first] * rg_num_ / total_weight, m.second);
    }
    for (auto &m : host_sec_rgs_map) {
        ASSERT_EQ(
                host_num_map[m.first] * rg_num_ * (replica_num_ - 1)
                        / total_weight,
                m.second);
    }
}

TEST_F(RGAllocaterTest, TestAllocaterByHostWithDifferWeight) {
    rg_allocater_ = std::shared_ptr<RGAllocaterByHost>(new RGAllocaterByHost());
    rg_allocater_->init(ess1_, 1073741824);
    for (int i = 0; i < rg_num_; i++) {
        auto es = rg_allocater_->select_primary();
        rg_es_map_[i].push_back(es->get_id());
    }

    for (int i = 1; i < replica_num_; i++) {
        for (int j = 0; j < rg_num_; j++) {
            auto es = rg_allocater_->select_secondary(rg_es_map_[j]);
            rg_es_map_[j].push_back(es->get_id());
        }
    }

    std::map<std::string, int> host_rgs_map;
    std::map<std::string, int> host_pri_rgs_map;
    std::map<std::string, int> host_sec_rgs_map;
    std::map<std::string, int> host_num_map;
    int total_weight = 0;
    for (auto &es : ess1_) {
        host_rgs_map[es->get_host()] += es->get_rgs();
        host_pri_rgs_map[es->get_host()] += es->get_pri_rgs();
        host_sec_rgs_map[es->get_host()] += es->get_sec_rgs();
        host_num_map[es->get_host()] += es->get_weight();
        ;
        total_weight += es->get_weight();
    }

    // for (auto &m : host_rgs_map) {
    //	ASSERT_EQ(host_num_map[m.first] * rg_num_ * replica_num_ / es_num_,
    // m.second);
    //}
    for (auto &m : host_pri_rgs_map) {
        ASSERT_EQ(host_num_map[m.first] * rg_num_ / total_weight, m.second);
    }
    // for (auto &m : host_sec_rgs_map) {
    //	ASSERT_EQ(host_num_map[m.first] * rg_num_ * (replica_num_ - 1) /
    // es_num_, m.second);
    //}
}

TEST_F(RGAllocaterTest, TestAllocaterByRack) {
    rg_allocater_ = std::shared_ptr<RGAllocaterByRack>(new RGAllocaterByRack());
    rg_allocater_->init(ess_, 1073741824);
    for (int i = 0; i < rg_num_; i++) {
        auto es = rg_allocater_->select_primary();
        rg_es_map_[i].push_back(es->get_id());
    }

    for (int i = 1; i < replica_num_; i++) {
        for (int j = 0; j < rg_num_; j++) {
            auto es = rg_allocater_->select_secondary(rg_es_map_[j]);
            rg_es_map_[j].push_back(es->get_id());
        }
    }

    std::map<std::string, int> rack_rgs_map;
    std::map<std::string, int> rack_pri_rgs_map;
    std::map<std::string, int> rack_sec_rgs_map;
    for (auto &es : ess_) {
        rack_rgs_map[es->get_rack()] += es->get_rgs();
        rack_pri_rgs_map[es->get_rack()] += es->get_pri_rgs();
        rack_sec_rgs_map[es->get_rack()] += es->get_sec_rgs();
    }

    for (auto &m : rack_rgs_map) {
        ASSERT_EQ(rg_num_ * replica_num_ / rack_num_, m.second);
    }
    for (auto &m : rack_pri_rgs_map) {
        ASSERT_EQ(rg_num_ / rack_num_, m.second);
    }
    for (auto &m : rack_sec_rgs_map) {
        ASSERT_EQ(rg_num_ * (replica_num_ - 1) / rack_num_, m.second);
    }
}

TEST_F(RGAllocaterTest, TestAllocaterByRackWithDifferWeight) {
    rg_allocater_ = std::shared_ptr<RGAllocaterByRack>(new RGAllocaterByRack());
    rg_allocater_->init(ess1_, 1073741824);
    for (int i = 0; i < rg_num_; i++) {
        auto es = rg_allocater_->select_primary();
        rg_es_map_[i].push_back(es->get_id());
    }

    for (int i = 1; i < replica_num_; i++) {
        for (int j = 0; j < rg_num_; j++) {
            auto es = rg_allocater_->select_secondary(rg_es_map_[j]);
            rg_es_map_[j].push_back(es->get_id());
        }
    }

    std::map<std::string, int> rack_rgs_map;
    std::map<std::string, int> rack_pri_rgs_map;
    std::map<std::string, int> rack_sec_rgs_map;
    std::map<std::string, int> rack_weight_map;
    int total_weight = 0;
    for (auto &es : ess1_) {
        rack_rgs_map[es->get_rack()] += es->get_rgs();
        rack_pri_rgs_map[es->get_rack()] += es->get_pri_rgs();
        rack_sec_rgs_map[es->get_rack()] += es->get_sec_rgs();
        rack_weight_map[es->get_rack()] += es->get_weight();
        total_weight += es->get_weight();
    }

    // for (auto &m : rack_rgs_map) {
    //	ASSERT_EQ(rack_weight_map[m.first] * rg_num_ * replica_num_ /
    // total_weight, m.second);
    //}
    for (auto &m : rack_pri_rgs_map) {
        ASSERT_EQ(rack_weight_map[m.first] * rg_num_ / total_weight, m.second);
    }
    // for (auto &m : rack_sec_rgs_map) {
    //	ASSERT_EQ(rack_weight_map[m.first] * rg_num_ * (replica_num_ - 1) /
    // total_weight, m.second);
    //}
}

}  // namespace
}  // namespace extentmanager
}  // namespace cyprestore
