

/*
 * Copyright 2021 JDD authors.
 * @zhangliang
 */
#include "stream/brpc_es_wrapper.h"

#include <bthread/bthread.h>

#include "common/builtin.h"
#include "common/connection_pool.h"
#include "common/fast_signal.h"
#include "common/ring.h"
#include "stream/rbd_stream_handle_impl.h"
#include "utils/chrono.h"

namespace cyprestore {
namespace clients {

bvar::LatencyRecorder g_latency_sdk_consume("cypre_sdk_consume");
bvar::LatencyRecorder g_latency_sdk_usercb("cypre_sdk_usercb");
bvar::LatencyRecorder g_latency_sdk_quetime("cypre_sdk_quetime");
bvar::LatencyRecorder g_latency_read_e2etime("cypre_read_e2etime");
bvar::LatencyRecorder g_latency_write_e2etime("cypre_write_e2etime");

static inline void brpc_iobuf_userdata_dummy_deleter(void *buf) {
    (void)(buf);
}

class BrpcEsCaller {
public:
    BrpcEsCaller(bool isReader) : isReader_(isReader), isNullAsyncIo_(false) {}
    virtual ~BrpcEsCaller() {}

    bool IsReader() const {
        return isReader_;
    }
    bool IsNullAsyncIo() const {
        return isNullAsyncIo_;
    }
    void Test_SetNullAsyncIo() {
        isNullAsyncIo_ = true;
    }

    virtual uint64_t HashKey() const = 0;
    virtual int AsyncCall() = 0;
    virtual int SyncCall() = 0;
    virtual void SetExpired() = 0;

    void OnEnqueue() {
        utils::Chrono::GetTime(&tenque_);
    }
    void OnDequeue(const struct timespec &tt) {
        g_latency_sdk_quetime
                << utils::Chrono::TimeSinceUs(&tenque_, (struct timespec *)&tt);
        tdeque_ = tt;
    }

private:
    const bool isReader_;
    bool isNullAsyncIo_;
protected:
    struct timespec tenque_;
    struct timespec tdeque_;
};

class BrpcEsReader : public BrpcEsCaller {
public:
    BrpcEsReader(
            const ExtentStreamOptions &eopts, common::ConnectionPtr &conn,
            ReadRequest *req, google::protobuf::Closure *callback);
    virtual ~BrpcEsReader();

    virtual uint64_t HashKey() const {
        return (uint64_t)eopts_.extent_idx;
    }
    virtual int AsyncCall();
    virtual int SyncCall();
    virtual void SetExpired();

private:
    void onReadDone(
            brpc::Controller *cntl, extentserver::pb::ReadResponse *resp,
            ReadRequest *req, google::protobuf::Closure *callback);

    const ExtentStreamOptions &eopts_;
    common::ConnectionPtr conn_;
    ReadRequest *req_;
    google::protobuf::Closure *cb_;
};

class BrpcEsWriter : public BrpcEsCaller {
public:
    BrpcEsWriter(
            const ExtentStreamOptions &eopts, common::ConnectionPtr &conn,
            WriteRequest *req, google::protobuf::Closure *callback);
    ~BrpcEsWriter();

    virtual uint64_t HashKey() const {
        return (uint64_t)eopts_.extent_idx;
    }
    virtual int AsyncCall();
    virtual int SyncCall();
    virtual void SetExpired();

private:
    void onWriteDone(
            brpc::Controller *cntl, extentserver::pb::WriteResponse *resp,
            WriteRequest *req, google::protobuf::Closure *callback);

    const ExtentStreamOptions &eopts_;
    common::ConnectionPtr conn_;
    WriteRequest *req_;
    google::protobuf::Closure *cb_;
};

class BrpcSenderWorker {
public:
    BrpcSenderWorker(int thread_size, int ring_size_power)
            : stoped_(false), thread_size_(thread_size),
              ring_size_power_(ring_size_power), tids_(NULL), ctxs_(NULL) {}
    ~BrpcSenderWorker() {
        Stop();
    }
    int Start(const std::vector<int> &affinity);
    void Stop();
    inline int Push(BrpcEsCaller *caller);

private:
    static void *sender_loop(void *arg);

    struct sender_ctx_t {
        Ring *wq;
        common::FastSignal event;
        volatile bool stop;
    };
    std::atomic<bool> stoped_;
    const int thread_size_;
    const int ring_size_power_;
    pthread_t *tids_;
    struct sender_ctx_t *ctxs_;
};

int BrpcSenderWorker::Start(const std::vector<int> &affinity) {
    tids_ = new pthread_t[thread_size_];
    ctxs_ = new struct sender_ctx_t[thread_size_];
    for (int i = 0; i < thread_size_; i++) {
        char name[32] = "";
        snprintf(name, sizeof(name) - 1, "brs:%d", i);
        ctxs_[i].wq = new Ring(name, Ring::RING_MP_SC, 1 << ring_size_power_);
        ctxs_[i].wq->Init();
        ctxs_[i].stop = false;
    }
    stoped_ = false;  // set flag
    for (int i = 0, icpu = 0; i < thread_size_; i++, icpu++) {
        int rv = pthread_create(&tids_[i], NULL, sender_loop, &ctxs_[i]);
        if (rv != 0) {
            LOG(ERROR) << "Create sender thread failed, rv=" << rv;
            return common::CYPRE_C_INTERNAL_ERROR;
        }
        if (affinity.empty()) {
            continue;
        }
        if (icpu >= (int)affinity.size()) {
            icpu = 0;
        }
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(affinity[icpu], &cpuset);
        rv = pthread_setaffinity_np(tids_[i], sizeof(cpuset), &cpuset);
        if (rv != 0) {
            LOG(ERROR) << "Set sender thread affinity failed, rv=" << rv;
            return common::CYPRE_C_INTERNAL_ERROR;
        }
    }
    return common::CYPRE_OK;
}
void BrpcSenderWorker::Stop() {
    if (stoped_.exchange(true)) {
        return;
    }
    for (int i = 0; i < thread_size_; i++) {
        ctxs_[i].stop = true;
        ctxs_[i].event.Broadcast(1);
    }
    for (int i = 0; i < thread_size_; i++) {
        pthread_join(tids_[i], NULL);
    }
    for (int i = 0; i < thread_size_; i++) {
        delete ctxs_[i].wq;
    }
    delete[] tids_;
    delete[] ctxs_;
    tids_ = NULL;
    ctxs_ = NULL;
}

int BrpcSenderWorker::Push(BrpcEsCaller *caller) {
    int idx = caller->HashKey() & (thread_size_ - 1);
    struct sender_ctx_t *ctx = &ctxs_[idx];

    caller->OnEnqueue();
    Status s = ctx->wq->Enqueue(caller);
    int counter = 0;
    while (!s.ok()) {
        if (++counter == 128) {
            usleep(10);
            counter = 0;
        }
        s = ctx->wq->Enqueue(caller);
    }
    ctx->event.Signal();
    return common::CYPRE_OK;
}

void *BrpcSenderWorker::sender_loop(void *arg) {
    void *tmp = NULL;
    struct sender_ctx_t *ctx = (struct sender_ctx_t *)arg;
    struct timespec te, ts;
    while (true) {
        if (ctx->stop) break;
        ctx->event.Wait();  //&ctx->stop);
        while (true) {
            Status s = ctx->wq->Dequeue(&tmp);
            if (!s.ok()) break;
            utils::Chrono::GetTime(&ts);
            BrpcEsCaller *caller = (BrpcEsCaller *)tmp;
            caller->OnDequeue(ts);
            caller->AsyncCall();
            utils::Chrono::GetTime(&te);
            g_latency_sdk_consume << utils::Chrono::TimeSinceUs(&ts, &te);
        }
    }
    // expire all pending requests
    while (!ctx->wq->RingEmpty()) {
        Status s = ctx->wq->Dequeue(&tmp);
        if (!s.ok()) {
            break;
        }
        BrpcEsCaller *caller = (BrpcEsCaller *)tmp;
        caller->SetExpired();
    }
    return NULL;
}

int BrpcEsWrapper::StartSenderWorker(
        int thread_size, int ring_size_power, const std::vector<int> &affinity,
        BrpcSenderWorker **sender) {
    if (ring_size_power > 30 || ring_size_power < 10) {
        LOG(ERROR)
                << "to big ring size, ring_size_power should be in [10 ~ 30]";
        return common::CYPRE_ER_INVALID_ARGUMENT;
    }
    if (thread_size < 0 || thread_size > 64) {
        LOG(ERROR) << "invalid thread size, should be in [1 ~ 64]";
        return common::CYPRE_ER_INVALID_ARGUMENT;
    }
    if ((thread_size & (thread_size - 1)) != 0) {
        LOG(ERROR) << "thread size should be power of 2";
        return common::CYPRE_ER_INVALID_ARGUMENT;
    }
    BrpcSenderWorker *sw = new BrpcSenderWorker(thread_size, ring_size_power);
    int rv = sw->Start(affinity);
    if (rv == common::CYPRE_OK) {
        *sender = sw;
    } else {
        delete sw;
        LOG(ERROR) << "Start Brpc Sender Failed. rv:" << rv;
    }
    return rv;
}
void BrpcEsWrapper::StopSenderWorker(BrpcSenderWorker *sender) {
    if (sender != NULL) {
        sender->Stop();
        delete sender;
    }
}

int BrpcEsWrapper::AsyncRead(
        const common::ESInstance &es, ReadRequest *req,
        google::protobuf::Closure *callback) {
    auto conn = sopts_.conn_pool->GetConnection(es.es_id);
    if (unlikely(conn == nullptr)) {
        conn = sopts_.conn_pool->NewConnection(
                es.es_id, es.public_ip, es.public_port);
    }
    if (unlikely(conn == nullptr)) {
        LOG(ERROR) << "Couldn't connnect to " << es.address()
                   << ", extent_id:" << eopts_.extent_id;
        req->is_done = true;
        req->status = common::CYPRE_C_INTERNAL_ERROR;
        return req->status;
    }
    BrpcEsReader *reader = new BrpcEsReader(eopts_, conn, req, callback);
    if (likely(callback != NULL)) {
        int rv = sopts_.brpc_sender->Push(reader);  // reader->AsyncCall();
        if (rv != common::CYPRE_OK) {
            delete reader;
            req->is_done = true;
            req->status = common::CYPRE_C_INTERNAL_ERROR;
            return req->status;
        }
        return common::CYPRE_OK;
    }
    return reader->SyncCall();
}

int BrpcEsWrapper::AsyncWrite(
        const common::ESInstance &es, WriteRequest *req,
        google::protobuf::Closure *callback) {
    auto conn = sopts_.conn_pool->GetConnection(es.es_id);
    if (unlikely(conn == nullptr)) {
        conn = sopts_.conn_pool->NewConnection(
                es.es_id, es.public_ip, es.public_port);
    }
    if (unlikely(conn == nullptr)) {
        LOG(ERROR) << "Couldn't connnect to " << es.address()
                   << ", extent_id:" << eopts_.extent_id;
        req->is_done = true;
        req->status = common::CYPRE_C_INTERNAL_ERROR;
        return req->status;
    }
    BrpcEsWriter *writer = new BrpcEsWriter(eopts_, conn, req, callback);
    if (likely(callback != NULL)) {
        int rv = sopts_.brpc_sender->Push(writer);  // writer->AsyncCall();
        if (rv != common::CYPRE_OK) {
            delete writer;
            req->is_done = true;
            req->status = common::CYPRE_C_INTERNAL_ERROR;
            return req->status;
        }
        return common::CYPRE_OK;
    }
    return writer->SyncCall();
}

//////////////////class BrpcEsReader/////////////////
BrpcEsReader::BrpcEsReader(
        const ExtentStreamOptions &eopts, common::ConnectionPtr &conn,
        ReadRequest *req, google::protobuf::Closure *callback)
        : BrpcEsCaller(true), eopts_(eopts), conn_(conn), req_(req),
          cb_(callback) {}

BrpcEsReader::~BrpcEsReader() {}

void BrpcEsReader::SetExpired() {
    if (cb_) {
        req_->is_done = true;
        req_->status = common::CYPRE_C_DEVICE_CLOSED;
        cb_->Run();
    }
    delete this;
}

int BrpcEsReader::AsyncCall() {
    // send request
    extentserver::pb::ReadRequest request;
    request.set_extent_id(eopts_.extent_id);
    request.set_offset(req_->real_offset);
    request.set_size(req_->real_len);
    request.set_header_crc32(req_->header_crc32_);
    extentserver::pb::ReadResponse *response =
            new extentserver::pb::ReadResponse();
    brpc::Controller *cntl = new brpc::Controller();
    google::protobuf::Closure *done = brpc::NewCallback(
            this, &BrpcEsReader::onReadDone, cntl, response, req_, cb_);
    if (unlikely(IsNullAsyncIo())) {
        done->Run();
    } else {
        extentserver::pb::ExtentIOService_Stub stub(conn_->channel.get());
        stub.Read(cntl, &request, response, done);
    }
    return common::CYPRE_OK;
}

int BrpcEsReader::SyncCall() {
    // send request
    brpc::Controller *cntl = new brpc::Controller();
    extentserver::pb::ExtentIOService_Stub stub(conn_->channel.get());
    extentserver::pb::ReadRequest request;
    extentserver::pb::ReadResponse *response =
            new extentserver::pb::ReadResponse();

    request.set_extent_id(eopts_.extent_id);
    request.set_offset(req_->real_offset);
    request.set_size(req_->real_len);
    request.set_header_crc32(req_->header_crc32_);
    // sync mode
    stub.Read(cntl, &request, response, NULL);
    onReadDone(cntl, response, req_, NULL);
    return req_->status;
}

void BrpcEsReader::onReadDone(
        brpc::Controller *cntl, extentserver::pb::ReadResponse *resp,
        ReadRequest *req, google::protobuf::Closure *callback) {
	struct timespec curtime;
    utils::Chrono::GetTime(&curtime);
	g_latency_read_e2etime << utils::Chrono::TimeSinceUs(&tdeque_, &curtime);
    int rc = common::CYPRE_OK;
    if (req->is_done.exchange(true, std::memory_order_relaxed)) {
        return;  // avoid double call
    }
    std::unique_ptr<brpc::Controller> cntl_guard(cntl);
    std::unique_ptr<extentserver::pb::ReadResponse> response_guard(resp);
    if (cntl->Failed()) {
        rc = common::CYPRE_ER_NET_ERROR;
        LOG(ERROR) << "Couldn't send read request, " << cntl->ErrorText()
                   << ", Extent:" << eopts_.extent_id;
    } else if (resp->status().code() != 0) {
        rc = resp->status().code();
        LOG(ERROR) << "Couldn't read extent, " << resp->status().message()
                   << ", Extent:" << eopts_.extent_id << ", rc:" << rc;
    }

    if (common::CYPRE_OK == resp->status().code()) {
        const butil::IOBuf &io_buf = cntl->response_attachment();
        io_buf.copy_to(req->buf);
        if (resp->has_crc32()) {
            req->data_crc32_ = resp->crc32();
            req->has_data_crc32_ = true;
        }
    }
    req->status = rc;
    if (likely(callback != NULL)) {
        struct timespec te, ts;
        utils::Chrono::GetTime(&ts);
        callback->Run();
        utils::Chrono::GetTime(&te);
        g_latency_sdk_usercb << utils::Chrono::TimeSinceUs(&ts, &te);
    }
    delete this;
}

/////////////////class BrpcEsWriter//////////////
BrpcEsWriter::BrpcEsWriter(
        const ExtentStreamOptions &eopts, common::ConnectionPtr &conn,
        WriteRequest *req, google::protobuf::Closure *callback)
        : BrpcEsCaller(false), eopts_(eopts), conn_(conn), req_(req),
          cb_(callback) {}

BrpcEsWriter::~BrpcEsWriter() {}

void BrpcEsWriter::SetExpired() {
    if (cb_) {
        req_->is_done = true;
        req_->status = common::CYPRE_C_DEVICE_CLOSED;
        cb_->Run();
    }
    delete this;
}

int BrpcEsWriter::AsyncCall() {
    // send request
    brpc::Controller *cntl = new brpc::Controller();
    extentserver::pb::WriteRequest request;
    extentserver::pb::WriteResponse *response =
            new extentserver::pb::WriteResponse();

    request.set_extent_id(eopts_.extent_id);
    request.set_offset(req_->real_offset);
    request.set_size(req_->real_len);
    request.set_crc32(req_->data_crc32_);
    request.set_header_crc32(req_->header_crc32_);

    // TODO: Don't using zero-copy function utill brpc timeout problem is figured out.
    //cntl->request_attachment().append_user_data(
    //        const_cast<void *>(req_->buf), req_->real_len,
    //        brpc_iobuf_userdata_dummy_deleter);
    cntl->request_attachment().append(
            const_cast<void *>(req_->buf), req_->real_len);
    google::protobuf::Closure *done = brpc::NewCallback(
            this, &BrpcEsWriter::onWriteDone, cntl, response, req_, cb_);
    if (unlikely(IsNullAsyncIo())) {
        done->Run();
    } else {
        extentserver::pb::ExtentIOService_Stub stub(conn_->channel.get());
        stub.Write(cntl, &request, response, done);
    }
    return common::CYPRE_OK;
}

int BrpcEsWriter::SyncCall() {
    // send request
    brpc::Controller *cntl = new brpc::Controller();
    extentserver::pb::ExtentIOService_Stub stub(conn_->channel.get());
    extentserver::pb::WriteRequest request;
    extentserver::pb::WriteResponse *response =
            new extentserver::pb::WriteResponse();

    request.set_extent_id(eopts_.extent_id);
    request.set_offset(req_->real_offset);
    request.set_size(req_->real_len);
    request.set_crc32(req_->data_crc32_);
    request.set_header_crc32(req_->header_crc32_);

    // TODO: Don't using zero-copy function utill brpc timeout problem is figured out.
    //cntl->request_attachment().append_user_data(
    //        const_cast<void *>(req_->buf), req_->real_len,
    //        brpc_iobuf_userdata_dummy_deleter);
    cntl->request_attachment().append(
            const_cast<void *>(req_->buf), req_->real_len);

    // sync mode
    stub.Write(cntl, &request, response, NULL);
    onWriteDone(cntl, response, req_, NULL);
    return req_->status;
}

void BrpcEsWriter::onWriteDone(
        brpc::Controller *cntl, extentserver::pb::WriteResponse *resp,
        WriteRequest *req, google::protobuf::Closure *callback) {
	struct timespec curtime;
    utils::Chrono::GetTime(&curtime);
	g_latency_write_e2etime << utils::Chrono::TimeSinceUs(&tdeque_, &curtime);
    if (req->is_done.exchange(true, std::memory_order_relaxed)) {
        return;  // avoid double call
    }
    std::unique_ptr<brpc::Controller> cntl_guard(cntl);
    std::unique_ptr<extentserver::pb::WriteResponse> response_guard(resp);
    int rc = common::CYPRE_OK;
    if (cntl->Failed()) {
        rc = common::CYPRE_ER_NET_ERROR;
        LOG(ERROR) << "Couldn't send write request, " << cntl->ErrorText()
				   << ", offset:" << req->real_offset
				   << ", len:" << req->real_len
				   << ", crc32:" << req->data_crc32_
				   << ", logic_offset:" << req->ureq->logic_offset
				   << ", logic_len:" << req->ureq->logic_len
				   << ", logic_crc32:" << req->ureq->data_crc32_
                   << ", extent_id:" << eopts_.extent_id;
    } else if (resp->status().code() != 0) {
        rc = resp->status().code();
        LOG(ERROR) << "Couldn't write extent, " << resp->status().message()
				   << ", offset:" << req->real_offset
				   << ", len:" << req->real_len
				   << ", crc32:" << req->data_crc32_
				   << ", logic_offset:" << req->ureq->logic_offset
				   << ", logic_len:" << req->ureq->logic_len
				   << ", logic_crc32:" << req->ureq->data_crc32_
                   << ", extent_id:" << eopts_.extent_id << ", rc:" << rc;
    }
    req->status = rc;
    if (likely(callback != NULL)) {
        struct timespec te, ts;
        utils::Chrono::GetTime(&ts);
        callback->Run();
        utils::Chrono::GetTime(&te);
        g_latency_sdk_usercb << utils::Chrono::TimeSinceUs(&ts, &te);
    }
    delete this;
}

///////////// Null Wrapper for test///////////////

int NullEsWrapper::AsyncRead(
        const common::ESInstance &es, ReadRequest *req,
        google::protobuf::Closure *callback) {
    auto conn = sopts_.conn_pool->GetConnection(es.es_id);
    if (unlikely(conn == nullptr)) {
        conn = sopts_.conn_pool->NewConnection(
                es.es_id, es.public_ip, es.public_port);
    }
    BrpcEsReader *reader = new BrpcEsReader(eopts_, conn, req, callback);
    reader->Test_SetNullAsyncIo();
    if (callback != NULL) {
        int rv = sopts_.brpc_sender->Push(reader);  // reader->AsyncCall(); //
        if (rv != common::CYPRE_OK) {
            delete reader;
            req->is_done = true;
            req->status = common::CYPRE_C_INTERNAL_ERROR;
            return req->status;
        }
        return common::CYPRE_OK;
    }
    delete reader;
    return common::CYPRE_OK;
}

int NullEsWrapper::AsyncWrite(
        const common::ESInstance &es, WriteRequest *req,
        google::protobuf::Closure *callback) {
    auto conn = sopts_.conn_pool->GetConnection(es.es_id);
    if (unlikely(conn == nullptr)) {
        conn = sopts_.conn_pool->NewConnection(
                es.es_id, es.public_ip, es.public_port);
    }
    BrpcEsWriter *writer = new BrpcEsWriter(eopts_, conn, req, callback);
    writer->Test_SetNullAsyncIo();
    if (callback != NULL) {
        int rv = sopts_.brpc_sender->Push(writer);  // writer->AsyncCall(); //
        if (rv != common::CYPRE_OK) {
            delete writer;
            req->is_done = true;
            req->status = common::CYPRE_C_INTERNAL_ERROR;
            return req->status;
        }
        return common::CYPRE_OK;
    }
    delete writer;
    return common::CYPRE_OK;
}

}  // namespace clients
}  // namespace cyprestore
