/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 */

#include "blob_instance.h"

#include <fcntl.h>
#include <sys/stat.h>

#include "bvar/bvar.h"
#include "common/error_code.h"
#include "config.h"
#include "libcypre/stream/rbd_stream_handle_impl.h"
#include "nbd_server.h"
#include "util.h"
#include "utils/chrono.h"

namespace cyprestore {
namespace nbd {
bvar::LatencyRecorder g_latency_cypre_nbd_read("cypre_nbd_read");
bvar::LatencyRecorder g_latency_cypre_nbd_write("cypre_nbd_write");
bvar::LatencyRecorder g_latency_cypre_nbd_submit("cypre_nbd_submit");
extern std::shared_ptr<NBDConfig> g_nbd_config;

BlobInstance::BlobInstance(
        const std::string &em_ip, int em_port, clients::CypreRBD *cyprerbd) {
    fd_ = -1;
    btestmem_ = false;
    btestfile_ = false;
    extent_manager_ip_ = em_ip;
    extent_manager_port_ = em_port;
    cyprerbd_ = cyprerbd;
}

BlobInstance::~BlobInstance() {
}

int BlobInstance::Open(const std::string &blobid) {
    if (blobid.length() <= 0)
        return -EINVAL;

    blobid_ = blobid;
    btestfile_ =
            (blobid == "test.iso" || blobid == "test.dat" || blobid[0] == '/');

    btestmem_ = (blobid == "testmem");
    if (btestmem_)
        return 0;
    else if (btestfile_) {
        fd_ = open(blobid_.c_str(), O_RDWR, 0600);
        if (fd_ <= 0) {
            LOG(ERROR) << "Fail to fopen test.iso error=" << errno
                       << strerror(errno);
            return -errno;
        }

        LOG(NOTICE) << "BlobInstance::Open success";
        return 0;
    } else {

        int rv = cyprerbd_->Open(blobid, handle_);
        if (rv < 0) {
            LOG(ERROR) << "BlobInstance::Open failed blobid=" << blobid << ": " << rv;
            return rv;
        }

        return 0;
    }
}

void BlobInstance::Close() {
    if (btestmem_)
        return;
    else if (btestfile_) {
        if (fd_ != 0) {
            close(fd_);
            fd_ = 0;
        }
    } else {
        if (handle_ != nullptr) {
            cyprerbd_->Close(handle_);
            handle_ = nullptr;
        }
    }
    fd_ = -1;
}

bool BlobInstance::Read(IOContext *ctx) {
    auto offset = ctx->request.from;
    auto length = ctx->request.len;
    LOG(INFO) << "try read offset=" << offset << " len=" << length;
    if (btestmem_) {
        return TestMemRead(ctx);
    } else if (btestfile_) {
        return TestFileRead(ctx);
    } else {
        int offyu = offset % CYPRE_BLOCK_SIZE;
        ctx->align_offset = offset - offyu;
        ctx->align_length = offyu + length;
        int lenyu = ctx->align_length % CYPRE_BLOCK_SIZE;
        if (lenyu > 0) ctx->align_length += CYPRE_BLOCK_SIZE - lenyu;
        auto align_offset = ctx->align_offset;
        auto align_length = ctx->align_length;

        ctx->data.reset(new char[sizeof(nbd_reply) + align_length]);
        char *buf = ctx->data.get() + sizeof(nbd_reply);

        if (g_nbd_config->nullio) {
            // 开始计时
            if (utils::Chrono::GetTime(&ctx->start_time) != 0) {
                LOG(ERROR) << "Couldn't gettime";
            }
        }

        int ret = handle_->AsyncRead(
                buf, align_length, align_offset, ctx->cb, ctx);

        if (g_nbd_config->nullio) {
            struct timespec submit_time;
            if (utils::Chrono::GetTime(&submit_time) == 0) {
                g_latency_cypre_nbd_submit << utils::Chrono::TimeSinceUs(
                        &ctx->start_time, &submit_time);
            }
        }

        LOG(INFO) << "read result=" << ret;
        return ret == 0;
    }
}

bool BlobInstance::Write(IOContext *ctx, bool flush) {
    auto offset = ctx->request.from;
    auto length = ctx->request.len;
    LOG(INFO) << "try write offset=" << offset << " len=" << length;
    if (btestmem_) {
        return TestMemWrite(ctx);
    } else if (btestfile_) {
        return TestFileWrite(ctx);
    } else {
        int offyu = offset % CYPRE_BLOCK_SIZE;
        ctx->align_offset = offset - offyu;
        ctx->align_length = offyu + length;
        int lenyu = ctx->align_length % CYPRE_BLOCK_SIZE;
        if (lenyu > 0) ctx->align_length += CYPRE_BLOCK_SIZE - lenyu;
        auto align_offset = ctx->align_offset;
        auto align_length = ctx->align_length;

        char *buf = NULL;
        if (offyu > 0 || length != align_length) {
            buf = new char[align_length];

            // read head
            int readlen = CYPRE_BLOCK_SIZE;
            if (align_length < 65536) readlen = align_length;
            int ret = handle_->Read(buf, readlen, align_offset);
            if (ret <= 0) {
                LOG(ERROR) << "BlobInstance::pread failed offset="
                           << align_offset << ", len=" << readlen
                           << ", ret=" << ret;
                return false;
            }

            if ((unsigned int)ret < align_length && lenyu > 0) {  // read tail
                char *endbuf = buf + align_length - CYPRE_BLOCK_SIZE;
                uint64_t endoffset = align_offset + align_length - CYPRE_BLOCK_SIZE;
                ret = handle_->Read(endbuf, CYPRE_BLOCK_SIZE, endoffset);
                if (ret <= 0) {
                    LOG(ERROR)
                            << "BlobInstance::pread failed offset=" << endoffset
                            << ", len=" << CYPRE_BLOCK_SIZE << ", ret=" << ret;
                    return false;
                }
            }

            char *oldbuf = ctx->data.get();
            memcpy(buf + offyu, oldbuf, length);
            ctx->data.reset(buf);
        }

        buf = ctx->data.get();

        if (g_nbd_config->nullio) {
            // 开始计时
            if (utils::Chrono::GetTime(&ctx->start_time) != 0) {
                LOG(ERROR) << "Couldn't gettime";
            }
        }

        int ret = handle_->AsyncWrite(
                buf, align_length, align_offset, ctx->cb, ctx);

        if (g_nbd_config->nullio) {
            struct timespec submit_time;
            if (utils::Chrono::GetTime(&submit_time) == 0) {
                g_latency_cypre_nbd_submit << utils::Chrono::TimeSinceUs(
                        &ctx->start_time, &submit_time);
            }
        }

        LOG(INFO) << "write result=" << ret;
        return ret == 0;
    }
}

void BlobInstance::Trim(IOContext *ctx) {
    ctx->ret = 0;
    ctx->cb(0, ctx);
}

void BlobInstance::Flush(IOContext *ctx) {
    ctx->ret = 0;
    ctx->cb(0, ctx);
}

uint64_t BlobInstance::GetBlobSize() {
    if (btestmem_) {
        return 1024 * 1024 * 1024;
    } else if (btestfile_) {
        off_t es;
        struct stat stat_buf;
        int error;

        stat_buf.st_size = 0;
        error = fstat(fd_, &stat_buf);
        if (!error) {
            /* always believe stat if a regular file as it might really
             * be zero length */
            if (S_ISREG(stat_buf.st_mode) || (stat_buf.st_size > 0)) {
                // LOG(NOTICE) << "GetBlobSize  " << stat_buf.st_size;
                return (uint64_t)stat_buf.st_size;
            }
        } else {
            LOG(ERROR) << "fstat failed: " << strerror(errno);
        }

        es = lseek(fd_, (off_t)0, SEEK_END);
        if (es > ((off_t)0)) {
            LOG(NOTICE) << "GetBlobSize lseek  " << es;
            return (uint64_t)es;
        } else {
            LOG(ERROR) << "lseek failed: " << strerror(errno);
            return UINT64_MAX;
        }
    } else {
        uint64_t blob_size = handle_->GetDeviceSize();
        LOG(INFO) << "GetBlobSize blobid=" << blobid_ << " size=" << blob_size;
        return blob_size;
    }
}

bool BlobInstance::TestMemRead(IOContext *ctx) {
    int buflen = sizeof(nbd_reply) + ctx->request.len;
    char *buf = new char[buflen];
    memset(buf, 0, buflen);
    ctx->data.reset(buf);
    ctx->cb(0, ctx);
    return true;
}

bool BlobInstance::TestMemWrite(IOContext *ctx) {
    ctx->cb(0, ctx);
    return true;
}

bool BlobInstance::TestFileRead(IOContext *ctx) {
    auto offset = ctx->request.from;
    auto len = ctx->request.len;
    auto buflen = sizeof(nbd_reply) + len;
    ctx->data.reset(new char[buflen]);
    uint8_t *buf = (uint8_t *)ctx->data.get() + sizeof(nbd_reply);
    int ret = 0;

    while (len > 0) {
        ret = pread(fd_, buf, len, offset);
        if (ret <= 0) break;

        offset += ret;
        buf += ret;
        len -= ret;
    }

    if (ret < 0 || len != 0) {
        ctx->ret = errno;
        LOG(ERROR) << "TestFileRead error offset=" << offset
                   << ", len=" << len << ", ret=" << ret;
        return false;
    } else {
        ctx->ret = 0;
        ctx->cb(0, ctx);
        return true;
    }
}

bool BlobInstance::TestFileWrite(IOContext *ctx, bool flush) {
    auto offset = ctx->request.from;
    auto len = ctx->request.len;
    uint8_t *buf = (uint8_t *)ctx->data.get();
    int ret = 0;

    while (len > 0 && (ret = pwrite(fd_, buf, len, offset)) > 0) {
        offset += ret;
        buf += ret;
        len -= ret;
    }
    if (flush == true) fdatasync(fd_);

    if (ret < 0 || len != 0) {
        ctx->ret = errno;
        LOG(ERROR) << "TestFileWrite error offset=" << offset
                   << ", len=" << len << ", ret=" << ret;
        return false;
    } else {
        ctx->ret = 0;
        ctx->cb(0, ctx);
        return true;
    }
}

}  // namespace nbd
}  // namespace cyprestore
