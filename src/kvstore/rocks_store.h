/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#ifndef CYPRESTORE_KVSTORE_ROCKSSTORE_H_
#define CYPRESTORE_KVSTORE_ROCKSSTORE_H_

#include <butil/logging.h>
#include <butil/macros.h>

#include <memory>
#include <string>
#include <vector>

#include "rocksdb/db.h"
#include "rocksdb/iterator.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "rocksdb/status.h"
#include "rocksdb/write_batch.h"

namespace cyprestore {
namespace kvstore {

class RocksStore;
typedef std::pair<std::string, std::string> KV;
typedef std::shared_ptr<RocksStore> RocksStorePtr;

class RocksStatus {
public:
    RocksStatus() {}
    explicit RocksStatus(const rocksdb::Status status) : status_(status) {}

    bool ok() const {
        return status_.ok();
    }
    bool IsNotFound() const {
        return status_.IsNotFound();
    }

    std::string ToString() {
        return status_.ToString();
    }

private:
    rocksdb::Status status_;
};

class RocksWriteBatch {
public:
    RocksStatus Put(const std::string &key, const std::string &value) {
        rocksdb::Status status = batch_.Put(key, value);
        return RocksStatus(status);
    }

    RocksStatus Delete(const std::string &key) {
        rocksdb::Status status = batch_.Delete(key);
        return RocksStatus(status);
    }

    RocksStatus DeleteRange(const std::string &start, const std::string &end) {
        rocksdb::Status status = batch_.DeleteRange(start, end);
        return RocksStatus(status);
    }

    rocksdb::WriteBatch *data() {
        return &batch_;
    }

private:
    rocksdb::WriteBatch batch_;
};

class KVIterator {
public:
    KVIterator() = default;

    virtual ~KVIterator() = default;

    virtual bool Valid() const = 0;

    virtual void Next() = 0;

    virtual void Prev() = 0;

    virtual std::string key() const = 0;

    virtual std::string value() const = 0;
};

class RocksRangeIter : public KVIterator {
public:
    RocksRangeIter(
            rocksdb::Iterator *iter, const rocksdb::Slice &start,
            const rocksdb::Slice &end)
            : iter_(iter), start_(start), end_(end) {}

    ~RocksRangeIter() {}

    virtual bool Valid() const {
        return iter_ && iter_->Valid() && (iter_->key().compare(end_) < 0);
    }

    virtual void Next() {
        iter_->Next();
    }

    virtual void Prev() {
        iter_->Prev();
    }

    virtual std::string key() const {
        return std::string(iter_->key().data(), iter_->key().size());
    }

    virtual std::string value() const {
        return std::string(iter_->value().data(), iter_->value().size());
    }

private:
    std::unique_ptr<rocksdb::Iterator> iter_;
    rocksdb::Slice start_;
    rocksdb::Slice end_;
};

class RocksPrefixIter : public KVIterator {
public:
    RocksPrefixIter(rocksdb::Iterator *iter, const rocksdb::Slice &prefix)
            : iter_(iter), prefix_(prefix) {}

    ~RocksPrefixIter() {}

    virtual bool Valid() const {
        return iter_ && iter_->Valid() && (iter_->key().starts_with(prefix_));
    }

    virtual void Next() {
        iter_->Next();
    }

    virtual void Prev() {
        iter_->Prev();
    }

    virtual std::string key() const {
        return std::string(iter_->key().data(), iter_->key().size());
    }

    virtual std::string value() const {
        return std::string(iter_->value().data(), iter_->value().size());
    }

private:
    std::unique_ptr<rocksdb::Iterator> iter_;
    rocksdb::Slice prefix_;
};

struct RocksOption {
    RocksOption() : db_path(""), compact_threads(2) {}

    std::string db_path;
    int compact_threads;
};

class RocksStore {
public:
    // ctor
    explicit RocksStore(const RocksOption &rocks_option);
    ~RocksStore() {
        LOG(INFO) << "Release rocksdb on " << db_path_;
    }

    RocksStatus Open();
    RocksStatus Close();

    RocksStatus Put(const std::string &key, const std::string &value);
    RocksStatus MultiPut(const std::vector<KV> &kvs);

    RocksStatus Get(const std::string &key, std::string *value);
    std::vector<RocksStatus> MultiGet(
            const std::vector<std::string> &keys,
            std::vector<std::string> *values);

    RocksStatus Delete(const std::string &key);
    RocksStatus MultiDelete(const std::vector<std::string> &keys);
    RocksStatus DeleteRange(const std::string &begin, const std::string &end);

    RocksStatus ScanRange(
            const std::string &start, const std::string &end,
            std::unique_ptr<KVIterator> *iter);
    RocksStatus
    ScanPrefix(const std::string &prefix, std::unique_ptr<KVIterator> *iter);

    RocksStatus Write(RocksWriteBatch &write_batch);
    bool KeyExists(const std::string &key);

    std::string GetDBPath() const {
        return db_path_;
    }

private:
    DISALLOW_COPY_AND_ASSIGN(RocksStore);
    void InitRocksOptions();

    rocksdb::Options options_;
    std::string db_path_;
    int compact_threads_;
    std::unique_ptr<rocksdb::DB> db_;
};

}  // namespace kvstore
}  // namespace cyprestore

#endif  // CYPRESTORE_KVSTORE_ROCKSSTORE_H_
