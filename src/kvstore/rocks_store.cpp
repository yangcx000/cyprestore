/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "kvstore/rocks_store.h"

namespace cyprestore {
namespace kvstore {

RocksStore::RocksStore(const RocksOption &rocks_option) {
    db_path_ = rocks_option.db_path;
    compact_threads_ = rocks_option.compact_threads;
    InitRocksOptions();
}

void RocksStore::InitRocksOptions() {
    // XXX: add other itmes
    options_.create_if_missing = true;
    options_.IncreaseParallelism(compact_threads_);
    options_.OptimizeLevelStyleCompaction();
}

RocksStatus RocksStore::Open() {
    rocksdb::DB *db = nullptr;
    rocksdb::Status status = rocksdb::DB::Open(options_, db_path_, &db);
    if (!status.ok()) {
        if (status.IsCorruption() || status.IsIncomplete()
            || status.IsTryAgain()) {
            LOG(ERROR) << "try to repair rocksdb. [" << status.ToString()
                       << "] -> ["
                       << rocksdb::RepairDB(db_path_, options_).ToString()
                       << "]";
            status = rocksdb::DB::Open(options_, db_path_, &db);
        }

        if (!status.ok()) {
            LOG(ERROR) << "Couldn't open rocksdb on " << db_path_ << " : "
                       << status.ToString();
            return RocksStatus(status);
        }
    }

    db_.reset(db);
    LOG(INFO) << "Open rocksdb on " << db_path_;
    return RocksStatus(status);
}

RocksStatus RocksStore::Close() {
    rocksdb::Status status = db_->Close();
    if (!status.ok())
        LOG(ERROR) << "Couldn't close rocksdb on " << db_path_ << " : "
                   << status.ToString();

    return RocksStatus(status);
}

RocksStatus RocksStore::Put(const std::string &key, const std::string &value) {
    rocksdb::WriteOptions write_options;
    write_options.sync = true;
    rocksdb::Status status = db_->Put(write_options, key, value);
    return RocksStatus(status);
}

RocksStatus RocksStore::MultiPut(const std::vector<KV> &kvs) {
    rocksdb::WriteBatch updates;
    for (size_t i = 0; i < kvs.size(); ++i) {
        updates.Put(kvs[i].first, kvs[i].second);
    }

    rocksdb::WriteOptions write_options;
    write_options.sync = true;
    rocksdb::Status status = db_->Write(write_options, &updates);
    return RocksStatus(status);
}

RocksStatus RocksStore::Get(const std::string &key, std::string *value) {
    rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), key, value);
    return RocksStatus(status);
}

std::vector<RocksStatus> RocksStore::MultiGet(
        const std::vector<std::string> &keys,
        std::vector<std::string> *values) {
    std::vector<rocksdb::Slice> slice_keys;
    slice_keys.reserve(keys.size());
    for (size_t i = 0; i < keys.size(); ++i) {
        slice_keys.emplace_back(keys[i]);
    }

    std::vector<rocksdb::Status> ret =
            db_->MultiGet(rocksdb::ReadOptions(), slice_keys, values);
    std::vector<RocksStatus> rs;
    rs.reserve(ret.size());
    for (size_t i = 0; i < ret.size(); ++i) {
        rs.emplace_back(ret[i]);
    }
    return rs;
}

RocksStatus RocksStore::Delete(const std::string &key) {
    rocksdb::WriteOptions write_options;
    write_options.sync = true;
    rocksdb::Status status = db_->Delete(write_options, key);
    return RocksStatus(status);
}

RocksStatus RocksStore::MultiDelete(const std::vector<std::string> &keys) {
    rocksdb::WriteBatch updates;
    for (size_t i = 0; i < keys.size(); ++i) {
        updates.Delete(keys[i]);
    }

    rocksdb::WriteOptions write_options;
    write_options.sync = true;
    rocksdb::Status status = db_->Write(write_options, &updates);
    return RocksStatus(status);
}

RocksStatus
RocksStore::DeleteRange(const std::string &begin, const std::string &end) {
    rocksdb::WriteOptions write_options;
    write_options.sync = true;
    rocksdb::Status status = db_->DeleteRange(
            write_options, db_->DefaultColumnFamily(), begin, end);
    return RocksStatus(status);
}

RocksStatus RocksStore::ScanRange(
        const std::string &start, const std::string &end,
        std::unique_ptr<KVIterator> *iter) {
    rocksdb::ReadOptions read_options;
    read_options.total_order_seek = true;
    auto it = db_->NewIterator(read_options);
    if (it) {
        it->Seek(rocksdb::Slice(start));
    }

    iter->reset(new RocksRangeIter(it, start, end));
    return RocksStatus(rocksdb::Status());
}

RocksStatus RocksStore::ScanPrefix(
        const std::string &prefix, std::unique_ptr<KVIterator> *iter) {
    rocksdb::ReadOptions read_options;
    read_options.prefix_same_as_start = true;
    read_options.total_order_seek = true;
    auto it = db_->NewIterator(read_options);
    if (it) {
        it->Seek(rocksdb::Slice(prefix));
    }

    iter->reset(new RocksPrefixIter(it, prefix));
    return RocksStatus(rocksdb::Status());
}

RocksStatus RocksStore::Write(RocksWriteBatch &write_batch) {
    rocksdb::WriteOptions write_options;
    write_options.sync = true;
    rocksdb::Status status = db_->Write(write_options, write_batch.data());
    return RocksStatus(status);
}

bool RocksStore::KeyExists(const std::string &key) {
    std::string value;
    return db_->KeyMayExist(rocksdb::ReadOptions(), key, &value);
}

}  // namespace kvstore
}  // namespace cyprestore
