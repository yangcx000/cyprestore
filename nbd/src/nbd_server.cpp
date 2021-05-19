/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 */

#include "nbd_server.h"

#include <inttypes.h>
#include <netinet/in.h>
#include <signal.h>

#include "bvar/bvar.h"
#include "config.h"
#include "util.h"
#include "utils/chrono.h"

namespace cyprestore {
namespace nbd {

extern std::shared_ptr<NBDConfig> g_nbd_config;
extern bvar::LatencyRecorder g_latency_cypre_nbd_read;
extern bvar::LatencyRecorder g_latency_cypre_nbd_write;

#define REQUEST_TYPE_MASK 0x0000ffff

static std::ostream &operator<<(std::ostream &os, const IOContext &ctx) {
    uint64_t hexhandle = 0;
    ::memcpy(&hexhandle, ctx.request.handle, sizeof(hexhandle));
    os << "[" << std::hex << Ntohll(hexhandle);

    switch (ctx.command) {
        case NBD_CMD_WRITE:
            os << " WRITE ";
            break;
        case NBD_CMD_READ:
            os << " READ ";
            break;
        case NBD_CMD_FLUSH:
            os << " FLUSH ";
            break;
        case NBD_CMD_TRIM:
            os << " TRIM ";
            break;
        default:
            os << " UNKNOWN(" << ctx.command << ") ";
            break;
    }

    os << ctx.request.from << "~" << ctx.request.len << " " << std::dec
       << ntohl(ctx.reply.error) << "]";

    return os;
}

void NBDAioCallback(int rc, void *vctx) {
    try {
        IOContext *ctx = static_cast<IOContext *>(vctx);
        NBDServer *server = ctx->server;

        LOG(INFO) << "NBDAioCallback offset=" << ctx->request.from
                   << " ret=" << rc;

        if (utils::Chrono::GetTime(&ctx->end_time) != 0) {
            LOG(ERROR) << "Couldn't gettime";
        }

        if (g_nbd_config->nullio) {
            if (ctx->command == NBD_CMD_READ)
                g_latency_cypre_nbd_read << utils::Chrono::TimeSinceUs(
                        &ctx->start_time, &ctx->end_time);
            else if (ctx->command == NBD_CMD_WRITE)
                g_latency_cypre_nbd_write << utils::Chrono::TimeSinceUs(
                        &ctx->start_time, &ctx->end_time);
        }

        ctx->ret = rc;
        if (ctx->ret != 0) {
            LOG(ERROR) << "align_offset=" << ctx->align_offset
                       << " align_length=" << ctx->align_length
                       << " error=" << rc;
            ctx->reply.error = htonl(rc > 0 ? rc : -rc);
        } else { // success
            ctx->reply.error = htonl(0);
        }
        server->OnRequestFinish(ctx);
    }
    catch (std::exception &e) {
        std::cerr << "NBDAioCallback exception:" << e.what() << std::endl;
        LOG(ERROR) << "NBDAioCallback exception: " << e.what();
    }
}

NBDServer::NBDServer(int sock, std::shared_ptr<BlobInstance> blobInstance)
        : sock_(sock), pending_request_counts_(0), started_(false),
          terminated_(false), blob_(blobInstance) {
    safe_io_ = std::make_shared<SafeIO>();
}

NBDServer::~NBDServer() {
    Shutdown();
}

void NBDServer::Start() {
    if (started_) {
        return;
    }

    started_ = true;
    reader_thread_ = std::thread(&NBDServer::ReaderFunc, this);
}

void NBDServer::WaitForDisconnect() {
    if (!started_) {
        return;
    }

    std::unique_lock<std::mutex> lk(disconnect_mtx_);
    if (!terminated_)
        disconnect_cond_.wait(lk);
}

void NBDServer::Shutdown() {
    if (!started_)
        return;
    LOG(NOTICE) << "going to shutdown, terminated " << terminated_;
    bool expected = false;
    if (terminated_.compare_exchange_strong(expected, true)) {
        shutdown(sock_, SHUT_RDWR);
    }
    if (reader_thread_.joinable()
            && std::this_thread::get_id() != reader_thread_.get_id()) {
        reader_thread_.join();
        started_ = false;
        LOG(NOTICE) << "NBDServer quit";
    }
}

void NBDServer::ReaderFunc() {
    ssize_t r = 0;
    bool disconnect = false;
    const int BufSize = 1048576;
    unsigned char buf[BufSize] = { 0 };
    uint32_t bufpos = 0;
    uint32_t begin = 0;
    const int RequestHeadSize = sizeof(nbd_request);
    struct nbd_request *request;
    uint32_t datalen = 0;
    int command = 0;
    uint32_t request_type;
    uint64_t request_from;
    uint32_t request_len;

    while (!terminated_) {
        LOG(INFO) << "read begin sock_=" << sock_ << " pos=" << bufpos;
        r = read(sock_, buf + bufpos, BufSize - bufpos);
        if (r < 0) {
            if (errno == EINTR) {
                continue;
            }
            LOG(ERROR) << "read error " << CppStrerror(errno)
                       << " sock=" << sock_;
            break;
        } else if (r == 0) {
            LOG(ERROR) << "end of connection sock=" << sock_;
            break;
        } else {
            bufpos += r;
            LOG(INFO) << "read end ret=" << r << " pos=" << bufpos;
        }

        if (bufpos < RequestHeadSize) continue;

        begin = 0;
        while (begin < bufpos) {
            if (begin + RequestHeadSize > bufpos) {
                if (begin > BufSize - RequestHeadSize) {
                    memcpy(buf, buf + begin, bufpos - begin);
                    bufpos = bufpos - begin;
                    begin = 0;
                }
                break;
            }

            request = (nbd_request *)(buf + begin);
            if (request->magic != htonl(NBD_REQUEST_MAGIC)) {
                LOG(ERROR) << "Invalid nbd request magic " << request->magic;
                disconnect = true;
                break;
            }

            request_type = ntohl(request->type);
            request_from = Ntohll(request->from);
            request_len = ntohl(request->len);

            command = request_type & REQUEST_TYPE_MASK;
            if (command == NBD_CMD_WRITE)
                datalen = request_len;
            else
                datalen = 0;

            LOG(INFO) << "OnRecvPack datalen=" << datalen
                       << " begin=" << begin << " bufpos=" << bufpos << " command=" << command;
            if (begin + RequestHeadSize + datalen < bufpos) {
                disconnect = !OnRecvPack(
                        request_type, request_from, request_len,
                        request->handle, buf + begin + RequestHeadSize,
                        datalen);
                begin += RequestHeadSize + datalen;
            } else if (begin + RequestHeadSize + datalen == bufpos) {
                disconnect = !OnRecvPack(
                        request_type, request_from, request_len,
                        request->handle, buf + begin + RequestHeadSize,
                        datalen);
                begin = 0;
                bufpos = 0;
                break;
            } else if (begin + RequestHeadSize + datalen > bufpos) {
                datalen = bufpos - begin - RequestHeadSize;
                disconnect = !OnRecvPack(
                        request_type, request_from, request_len,
                        request->handle, buf + begin + RequestHeadSize,
                        datalen);
                begin = 0;
                bufpos = 0;
                break;
            }

            if (disconnect) break;
        }

        if (disconnect) {
            LOG(NOTICE) << "ReaderFunc() disconnect";
            break;
        }
    }

    std::lock_guard<std::mutex> lk(disconnect_mtx_);
    disconnect_cond_.notify_all();
    LOG(NOTICE) << "ReaderFunc terminated!";
    blob_->Close();
    Shutdown();
}

bool NBDServer::OnRecvPack(
        uint32_t request_type, uint64_t request_from, uint32_t request_len,
        const char *request_handle, const unsigned char *buf, uint32_t buflen) {
    bool disconnect = false;

    std::unique_ptr<IOContext> ctx(new IOContext());
    ctx->server = this;
    ctx->request.type = request_type;
    ctx->request.from = request_from;
    ctx->request.len = request_len;
    LOG(INFO) << "nbd request: from=" << ctx->request.from
               << " len=" << ctx->request.len << " buflen=" << buflen;

    ctx->reply.magic = htonl(NBD_REPLY_MAGIC);
    memcpy(ctx->reply.handle, request_handle, sizeof(ctx->reply.handle));
    ctx->reply.error = 0;

    ctx->command = ctx->request.type & REQUEST_TYPE_MASK;

    switch (ctx->command) {
        case NBD_CMD_DISC:
            LOG(NOTICE) << "Receive DISC request";
            disconnect = true;
            break;
        case NBD_CMD_WRITE: {
            char *p = new char[ctx->request.len];
            ctx->data.reset(p);
            if (buflen > 0) memcpy(p, buf, buflen);
            LOG(INFO) << "nbd write: buflen=" << buflen << " request.len=" << ctx->request.len;
            if (buflen < ctx->request.len) {
                // 写请求未读完，继续读取数据
                ssize_t r = safe_io_->ReadExact(sock_, p + buflen, ctx->request.len - buflen);
                if (r < 0) {
                    LOG(ERROR) << "OnRecvPack failed to read nbd request data "
                               << CppStrerror(r);
                    disconnect = true;
                }
            }
        } break;
        case NBD_CMD_READ:
            break;
    }

    if (disconnect) {
        LOG(NOTICE) << "ReaderFunc() disconnect";
        return false;
    }

    OnRequestStart();

    IOContext *pctx = ctx.get();
    bool ret = StartAioRequest(ctx.release());
    if (!ret) {
        pctx->ret = -1;
        OnRequestFinish(pctx);
        return false;
    }

    return true;
}

void NBDServer::SendReply(IOContext *ctx) {
    LOG(INFO) << "SendReply offset=" << ctx->request.from;
    ssize_t r = 0;
    const size_t kReplySize = sizeof(struct nbd_reply);
    if (ctx->command == NBD_CMD_READ && ctx->reply.error == htonl(0)) {
        int offyu = ctx->request.from % CYPRE_BLOCK_SIZE;
        char *buf = ctx->data.get() + offyu;
        memcpy(buf, &ctx->reply, kReplySize);

        r = safe_io_->Write(sock_, buf, kReplySize + ctx->request.len);
        if (r < 0) {
            LOG(ERROR) << *ctx
                       << ": faield to write reply data : " << CppStrerror(r);
            return;
        }
    } else {
        r = safe_io_->Write(sock_, &ctx->reply, kReplySize);
        if (r < 0) {
            LOG(ERROR) << *ctx << ": failed to write reply header : "
                       << CppStrerror(r);
            return;
        }
    }
}

void NBDServer::OnRequestStart() {
    ++pending_request_counts_;
    LOG(INFO) << __func__ << " pending_request=" << pending_request_counts_;
}

void NBDServer::OnRequestFinish(IOContext *ctx) {
    pending_request_counts_--;
    LOG(INFO) << __func__ << " pending_request=" << pending_request_counts_;
    sock_spin_lock_.lock();
    SendReply(ctx);
    sock_spin_lock_.unlock();
    delete ctx;
}

bool NBDServer::StartAioRequest(IOContext *ctx) {
    bool ret = true;
    switch (ctx->command) {
        case NBD_CMD_WRITE:
            ctx->cb = NBDAioCallback;
            ret = blob_->Write(ctx);
            break;
        case NBD_CMD_READ:
            ctx->cb = NBDAioCallback;
            ret = blob_->Read(ctx);
            break;
        case NBD_CMD_FLUSH:
            ctx->cb = NBDAioCallback;
            blob_->Flush(ctx);
            break;
        case NBD_CMD_TRIM:
            ctx->cb = NBDAioCallback;
            blob_->Trim(ctx);
            break;
        default:
            LOG(ERROR) << "Invalid request type: " << *ctx;
            ret = false;
            break;
    }

    return ret;
}

}  // namespace nbd
}  // namespace cyprestore
