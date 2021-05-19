/*
 * Copyright 2020 JDD authors.
 * @feifei5
 *
 */

#include "utils/serializer.h"

#include <cereal/access.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <iostream>
#include <sstream>
#include <string>

#include "gtest/gtest.h"
#include "kvstore/rocks_store.h"

namespace cyprestore {
namespace extentmanager {
namespace {

class SerializerClass {
public:
    SerializerClass() = default;
    SerializerClass(const std::string &id, const std::string name, int num)
            : id_(id), name_(name), num_(num) {
        vec_.push_back("1");
        vec_.push_back("2");
        vec_.push_back("3");
    }
    ~SerializerClass() = default;
    std::string kv_key() {
        return "serializerClass-" + id_;
    }
    std::string ToString() {
        std::stringstream ss(
                "SerialiserClass message: ",
                std::ios_base::app | std::ios_base::out);
        ss << "id : " << id_ << ", name: " << name_ << ", num: " << num_;
        for (auto &v : vec_) {
            ss << ", " << v;
        }
        return ss.str();
    }
    void add() {
        num_++;
    }
    int num() {
        return num_;
    }

private:
    friend class cereal::access;
    template <class Archive>
    void serialize(Archive &archive, std::uint32_t const version) {
        archive(id_);
        archive(name_);
        if (version > 0) {
            archive(num_);
        }
        archive(vec_);
    }
    std::string id_;
    std::string name_;
    int num_;
    std::vector<std::string> vec_;
};

class SerializerTest : public ::testing::Test {
protected:
    void SetUp() override {
        kvstore::RocksOption rocks_option;
        rocks_option.db_path = "./" + std::to_string(time(NULL));
        kv_store_ = new kvstore::RocksStore(rocks_option);
        auto status = kv_store_->Open();
        ASSERT_TRUE(status.ok());
    }

    void TearDown() override {
        delete kv_store_;
    }

    kvstore::RocksStore *kv_store_;
    std::uint32_t version_ = 0;
};

TEST_F(SerializerTest, TestClassSerialize) {
    SerializerClass *sc = new SerializerClass("one", "xxx", 1);
    std::string expected_value = sc->ToString();
    std::string key = sc->kv_key();

    auto status = kv_store_->Put(
            key, utils::Serializer<SerializerClass>::Encode(*sc));
    ASSERT_TRUE(status.ok());
    delete sc;

    std::string value;
    status = kv_store_->Get(key, &value);
    ASSERT_TRUE(status.ok());

    SerializerClass *scDecode = new SerializerClass();
    utils::Serializer<SerializerClass>::Decode(value, *scDecode);
    EXPECT_EQ(expected_value, scDecode->ToString());
    std::cout << "expected_value: " << expected_value << std::endl;
    std::cout << "actual_value: " << scDecode->ToString() << std::endl;
    delete scDecode;
}

TEST_F(SerializerTest, TestSharedPtrSerialize) {
    auto sc = std::make_shared<SerializerClass>("one", "xxx", 1);
    std::string expected_value = sc->ToString();
    std::string key = sc->kv_key();

    auto status = kv_store_->Put(
            key, utils::Serializer<SerializerClass>::Encode(*sc.get()));
    ASSERT_TRUE(status.ok());

    std::string value;
    status = kv_store_->Get(key, &value);
    ASSERT_TRUE(status.ok());

    auto scDecode = std::make_shared<SerializerClass>();
    utils::Serializer<SerializerClass>::Decode(value, *scDecode.get());
    EXPECT_EQ(expected_value, scDecode->ToString());
}

TEST_F(SerializerTest, TestEncodePerf4k) {
    std::string name(4096, 'a');
    auto sc = std::make_shared<SerializerClass>("one", name, 1);
    // for 4k class, one encode cost 3.5 us
    for (int i = 0; i < 2000000; i++) {
        sc->add();
        utils::Serializer<SerializerClass>::Encode(*sc.get());
    }
}

TEST_F(SerializerTest, TestEncodePerf1k) {
    std::string name(1024, 'a');
    auto sc = std::make_shared<SerializerClass>("one", name, 1);
    // for 1k class, one encode cost 2.6 us
    for (int i = 0; i < 2000000; i++) {
        sc->add();
        utils::Serializer<SerializerClass>::Encode(*sc.get());
    }
}

TEST_F(SerializerTest, TestDecodePerf4k) {
    std::string name(4096, 'a');
    auto sc = std::make_shared<SerializerClass>("one", name, 1);
    std::string value = utils::Serializer<SerializerClass>::Encode(*sc.get());
    // for 4k class, one decode cost 3.5 us
    for (int i = 0; i < 2000000; i++) {
        std::string val = value;
        auto scDecode = std::make_shared<SerializerClass>();
        utils::Serializer<SerializerClass>::Decode(val, *scDecode.get());
    }
}

TEST_F(SerializerTest, TestDecodePerf1k) {
    std::string name(1024, 'a');
    auto sc = std::make_shared<SerializerClass>("one", name, 1);
    std::string value = utils::Serializer<SerializerClass>::Encode(*sc.get());
    // for 1k class, one decode cost 2.5 us
    for (int i = 0; i < 2000000; i++) {
        std::string val = value;
        auto scDecode = std::make_shared<SerializerClass>();
        utils::Serializer<SerializerClass>::Decode(val, *scDecode.get());
    }
}

TEST_F(SerializerTest, TestCerealCompatibility) {
    std::string name(1024, 'a');
    auto sc = std::make_shared<SerializerClass>("one", name, 1);
    std::string value = utils::Serializer<SerializerClass>::Encode(*sc.get());
    std::string val = value;
    auto scDecode = std::make_shared<SerializerClass>();
    utils::Serializer<SerializerClass>::Decode(val, *scDecode.get());
    EXPECT_EQ(1, scDecode->num());

    sc->add();
    value = utils::Serializer<SerializerClass>::Encode(*sc.get());
    std::string val1 = value;
    auto scDecode1 = std::make_shared<SerializerClass>();
    utils::Serializer<SerializerClass>::Decode(val1, *scDecode1.get());
    EXPECT_EQ(2, scDecode1->num());
}

}  // namespace
}  // namespace extentmanager
}  // namespace cyprestore

// this macro must be placed at global scope
CEREAL_CLASS_VERSION(cyprestore::extentmanager::SerializerClass, 1);
