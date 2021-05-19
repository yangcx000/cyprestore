#include "kvstore/rocks_store.h"

#include "gtest/gtest.h"
#include "utils/uuid.h"

namespace cyprestore {
namespace kvstore {
namespace {

class RocksStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        RocksOption rocks_option;
        rocks_option.db_path = "./" + std::to_string(time(NULL));
        rocks_store_ = new RocksStore(rocks_option);
        RocksStatus s = rocks_store_->Open();
        ASSERT_TRUE(s.ok());

        value_.assign(kValueSize_, 'z');
    }

    void TearDown() override {
        delete rocks_store_;
    }

    std::string GenKey() {
        return key_prefix_ + utils::UUID::Generate();
    }

    const size_t kCount_ = 10000;
    const size_t kValueSize_ = 4096;
    const size_t kBatch_ = 1000;
    std::string key_prefix_;
    std::string value_;
    RocksStore *rocks_store_;
};

// 100000 put cost 14s, almost 7100 qps, for 4k byte value
// 100000 put cost 13s, almost 7700 qps, for 100 byte value
TEST_F(RocksStoreTest, TestPut) {
    RocksStatus s;
    key_prefix_ = "single_put_";
    for (size_t i = 0; i < kCount_; ++i) {
        s = rocks_store_->Put(GenKey(), value_);
        EXPECT_TRUE(s.ok());
    }
}

TEST_F(RocksStoreTest, TestMultiPut) {
    RocksStatus s;
    std::vector<KV> kvs;
    key_prefix_ = "multi_put_";
    std::vector<std::string> keys;
    for (size_t i = 0; i < kCount_ / kBatch_; ++i) {
        for (size_t j = 0; j < kBatch_; ++j) {
            std::string key = GenKey();
            keys.push_back(key);
            kvs.push_back(std::make_pair(key, value_));
        }

        s = rocks_store_->MultiPut(kvs);
        EXPECT_TRUE(s.ok());
        kvs.clear();
    }
    std::string expected_value;
    for (auto &key : keys) {
        s = rocks_store_->Get(key, &expected_value);
        EXPECT_TRUE(s.ok());
    }
}

TEST_F(RocksStoreTest, TestGet) {
    RocksStatus s;
    key_prefix_ = "test_get_";
    std::vector<std::string> keys;
    std::string key;
    for (size_t i = 0; i < kCount_; ++i) {
        key = GenKey();
        s = rocks_store_->Put(key, value_);
        EXPECT_TRUE(s.ok());
        keys.push_back(key);
    }

    std::string expected_value;
    for (size_t i = 0; i < keys.size(); ++i) {
        s = rocks_store_->Get(keys[i], &expected_value);
        EXPECT_TRUE(s.ok());
        EXPECT_EQ(expected_value, value_);
    }
}

TEST_F(RocksStoreTest, TestMultiGet) {
    RocksStatus s;
    key_prefix_ = "test_multiget_";
    std::vector<std::string> keys;
    std::string key;
    for (size_t i = 0; i < kCount_; ++i) {
        key = GenKey();
        s = rocks_store_->Put(key, value_);
        EXPECT_TRUE(s.ok());
        keys.push_back(key);
    }

    std::vector<std::string> expected_values;
    std::vector<RocksStatus> results =
            rocks_store_->MultiGet(keys, &expected_values);
    for (size_t i = 0; i < results.size(); ++i) {
        EXPECT_TRUE(results[i].ok());
    }

    for (size_t i = 0; i < expected_values.size(); ++i) {
        EXPECT_EQ(expected_values[i], value_);
    }
}

TEST_F(RocksStoreTest, TestDelete) {
    RocksStatus s;
    key_prefix_ = "test_delete_";
    std::vector<std::string> keys;
    std::string key;
    for (size_t i = 0; i < kCount_; ++i) {
        key = GenKey();
        s = rocks_store_->Put(key, value_);
        EXPECT_TRUE(s.ok());
        keys.push_back(key);
    }

    for (size_t i = 0; i < keys.size(); ++i) {
        s = rocks_store_->Delete(keys[i]);
        EXPECT_TRUE(s.ok());
    }

    std::string expected_value;
    for (size_t i = 0; i < keys.size(); ++i) {
        s = rocks_store_->Get(keys[i], &expected_value);
        EXPECT_TRUE(s.IsNotFound());
    }
}

TEST_F(RocksStoreTest, TestMultiDelete) {
    RocksStatus s;
    key_prefix_ = "test_multidelete_";
    std::vector<std::string> keys;
    std::string key;
    for (size_t i = 0; i < kCount_; ++i) {
        key = GenKey();
        s = rocks_store_->Put(key, value_);
        EXPECT_TRUE(s.ok());
        keys.push_back(key);
    }

    s = rocks_store_->MultiDelete(keys);
    EXPECT_TRUE(s.ok());

    std::string expected_value;
    for (size_t i = 0; i < keys.size(); ++i) {
        s = rocks_store_->Get(keys[i], &expected_value);
        EXPECT_TRUE(s.IsNotFound());
    }
}

TEST_F(RocksStoreTest, TestDeleteRange) {
    RocksStatus s;
    key_prefix_ = "test_deleterange_";
    std::vector<std::string> keys;
    std::string key;
    for (size_t i = 0; i < kCount_; ++i) {
        key = GenKey();
        s = rocks_store_->Put(key, value_);
        EXPECT_TRUE(s.ok());
        keys.push_back(key);
    }

    std::sort(keys.begin(), keys.end());
    s = rocks_store_->DeleteRange(keys.front(), keys.back());
    EXPECT_TRUE(s.ok());

    std::string expected_value;
    for (size_t i = 0; i < keys.size() - 1; ++i) {
        s = rocks_store_->Get(keys[i], &expected_value);
        EXPECT_TRUE(s.IsNotFound());
    }
}

TEST_F(RocksStoreTest, TestScanRange) {
    RocksStatus s;
    key_prefix_ = "test_scanrange_";
    std::vector<std::string> keys;
    std::string key;
    for (size_t i = 0; i < kCount_; ++i) {
        key = GenKey();
        s = rocks_store_->Put(key, value_);
        EXPECT_TRUE(s.ok());
        keys.push_back(key);
    }

    std::sort(keys.begin(), keys.end());
    std::unique_ptr<KVIterator> iter;
    s = rocks_store_->ScanRange(keys.front(), keys.back(), &iter);
    EXPECT_TRUE(s.ok());

    size_t index = 0;
    while (iter->Valid()) {
        EXPECT_EQ(iter->key(), keys[index]);
        EXPECT_EQ(iter->value(), value_);
        ++index;
        iter->Next();
    }
}

TEST_F(RocksStoreTest, TestScanPrefix) {
    RocksStatus s;
    key_prefix_ = "test_scanprefix_";
    std::vector<std::string> keys;
    std::string key;
    for (size_t i = 0; i < kCount_; ++i) {
        key = GenKey();
        s = rocks_store_->Put(key, value_);
        EXPECT_TRUE(s.ok());
        keys.push_back(key);
    }

    std::sort(keys.begin(), keys.end());
    std::unique_ptr<KVIterator> iter;
    // 200 * 10000 cost 4 s
    // 4000 * 10000 cost 63 s
    // 10000 * 10000 cost 154 s
    // for (int i = 0; i < 4000; i++) {
    s = rocks_store_->ScanPrefix(key_prefix_, &iter);
    EXPECT_TRUE(s.ok());

    size_t index = 0;
    while (iter->Valid()) {
        EXPECT_EQ(iter->key(), keys[index]);
        EXPECT_EQ(iter->value(), value_);
        ++index;
        iter->Next();
    }
    //}
}

TEST_F(RocksStoreTest, TestWrite) {
    RocksStatus s;
    key_prefix_ = "test_write_";
    std::vector<std::string> keys;
    std::string key;
    RocksWriteBatch write_batch;
    for (size_t i = 0; i < kCount_; ++i) {
        key = GenKey();
        s = write_batch.Put(key, value_);
        EXPECT_TRUE(s.ok());
        keys.push_back(key);
    }

    s = rocks_store_->Write(write_batch);
    EXPECT_TRUE(s.ok());

    std::string expected_value;
    for (size_t i = 0; i < keys.size(); ++i) {
        s = rocks_store_->Get(keys[i], &expected_value);
        EXPECT_TRUE(s.ok());
    }
}

TEST_F(RocksStoreTest, TestKeyExists) {
    RocksStatus s;
    key_prefix_ = "test_keyexists_";
    std::vector<std::string> keys;
    std::string key;
    RocksWriteBatch write_batch;
    for (size_t i = 0; i < kCount_; ++i) {
        key = GenKey();
        s = write_batch.Put(key, value_);
        EXPECT_TRUE(s.ok());
        keys.push_back(key);
    }

    for (size_t i = 0; i < keys.size(); ++i) {
        bool ret = rocks_store_->KeyExists(keys[i]);
        EXPECT_TRUE(ret);
    }
}

}  // namespace
}  // namespace kvstore
}  // namespace cyprestore
