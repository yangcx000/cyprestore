/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 *
 */
#include "blobplus.h"

#include "extentserver/pb/extent.pb.h"
#include "plus.h"
#include "rpcclient/rpcclient_em.h"
#include "rpcclient/rpcclient_es.h"
#include "stringdict.h"

using namespace cyprestore;

namespace cyprestore {

StatusCode InitBrpcChannel(
        brpc::Channel &channel, const std::string &server_addr, int port) {

    brpc::ChannelOptions options;
    options.protocol = "baidu_std";
    options.connection_type =
            "single";  // // Possible values: "single", "pooled", "short".
    options.timeout_ms = 20000; /*milliseconds*/
    ;
    options.max_retry = 3;  // Default: 3
    if (channel.Init(server_addr.c_str(), port, &options) != 0) {
        LOG(ERROR) << "Fail to initialize channel " << server_addr + ":"
                   << port;
        return (STATUS_FAILED_PRECONDITION);
    }

    return (STATUS_OK);
}

StatusCode WriteBlob(
        const std::string &ExtentManagerIp, int ExtentManagerPort,
        const std::string &blob_id, uint64_t offset, const unsigned char *buf,
        uint32_t buflen, std::string &errmsg) {
    if (buf == NULL || buflen <= 0) {
        errmsg = "args is invalid";
        return (STATUS_INVALID_ARGUMENT);
    }

    int extent_idx = offset / EXTENT_SIZE;
    uint64_t extent_offset =
            offset
            - EXTENT_SIZE * extent_idx;  //当前extent实际需要写入数据的offset
    uint32_t ShouldWriteExtentLen = 0;  //当前extent实际需要写入数据的length
    uint32_t WritedExtentLen = 0;  //当前extent实际已经写入数据的length
    int extent_offset_yu = 0;
    const uint64_t MAX_OnceWriteLen = BLOCK_SIZE * 1024;
    uint32_t OnceWriteLen = MAX_OnceWriteLen;
    uint32_t TotalWriteLen = 0;

    ::cyprestore::common::pb::ExtentRouter router;
    ::cyprestore::common::pb::EsInstance esinstance;

    RpcClientEM rpcClientEM;
    if (rpcClientEM.Init(ExtentManagerIp, ExtentManagerPort) != STATUS_OK)
        return (STATUS_FAILED_PRECONDITION);

    unsigned char TempBuf[BLOCK_SIZE] = { 0 };
    unsigned char *pbuf;
    RpcClientES rpcClientES;
    std::string crc32;

    while (TotalWriteLen < buflen) {
        ShouldWriteExtentLen = EXTENT_SIZE - extent_offset;
        if (ShouldWriteExtentLen > buflen - TotalWriteLen)
            ShouldWriteExtentLen = buflen - TotalWriteLen;
        WritedExtentLen = 0;

        StatusCode ret =
                rpcClientEM.QueryRouter(blob_id, extent_idx, router, errmsg);
        if (ret != STATUS_OK) return (ret);

        esinstance = router.primary();
        if (rpcClientES.Init(esinstance.public_ip(), esinstance.public_port())
            != STATUS_OK)
            return (STATUS_FAILED_PRECONDITION);
        int Index = 0;  //分多段写 extent的次序
        while (WritedExtentLen < ShouldWriteExtentLen) {
            OnceWriteLen = ShouldWriteExtentLen - WritedExtentLen;
            if (OnceWriteLen > MAX_OnceWriteLen)
                OnceWriteLen = MAX_OnceWriteLen;
            if (OnceWriteLen % BLOCK_SIZE != 0
                && OnceWriteLen
                           > BLOCK_SIZE)  // OnceWriteLen必须是BLOCK_SIZE(4K)的整数倍
            {
                OnceWriteLen = OnceWriteLen - OnceWriteLen % BLOCK_SIZE;
            }

            extent_offset_yu = extent_offset % BLOCK_SIZE;
            if (extent_offset_yu != 0
                || OnceWriteLen < BLOCK_SIZE
                           != 0) {  //不能对齐，不足4K的情况时，先读出4K，修改后再写回去
                uint32_t Readlen = 0;
                uint64_t ReadOffset = extent_offset - extent_offset_yu;
                if (extent_offset_yu != 0
                    && OnceWriteLen > BLOCK_SIZE - extent_offset_yu)
                    OnceWriteLen = BLOCK_SIZE - extent_offset_yu;

                ret = rpcClientES.Read(
                        router.extent_id(), ReadOffset, TempBuf, BLOCK_SIZE,
                        Readlen, crc32, errmsg);
                if (ret != STATUS_OK) return (ret);
                memcpy(TempBuf + extent_offset_yu, buf + TotalWriteLen,
                       OnceWriteLen);

                printf("rpcClientES.Write Index=%d, extent_idx=%d, "
                       "extent_offset=%lu, OnceWriteLen=%d\r\n",
                       Index, extent_idx, extent_offset, OnceWriteLen);
                ret = rpcClientES.Write(
                        router.extent_id(), ReadOffset, TempBuf, BLOCK_SIZE, "",
                        errmsg);
                if (ret != STATUS_OK) return (ret);
            } else {
                pbuf = (unsigned char *)buf + TotalWriteLen;
                //??extent_offset必须是BLOCK_SIZE的整数倍
                printf("rpcClientES.Write Index=%d, extent_idx=%d, "
                       "extent_offset=%lu, OnceWriteLen=%d\r\n",
                       Index, extent_idx, extent_offset, OnceWriteLen);
                ret = rpcClientES.Write(
                        router.extent_id(), extent_offset, pbuf, OnceWriteLen,
                        "", errmsg);
                if (ret != STATUS_OK) return (ret);
            }

            Index++;
            extent_offset += OnceWriteLen;
            WritedExtentLen += OnceWriteLen;
            TotalWriteLen += OnceWriteLen;
        }
        extent_idx++;
        extent_offset = 0;
    }

    return (STATUS_OK);
}

//一次最大读 64M
StatusCode ReadBlob(
        const std::string &ExtentManagerIp, int ExtentManagerPort,
        const std::string &blob_id, uint64_t offset, unsigned char *buf,
        uint32_t bufsize, uint32_t &retreadlen, std::string &errmsg) {
    retreadlen = 0;
    if (buf == NULL || bufsize <= 0) {
        errmsg = "args is invalid";
        return (STATUS_INVALID_ARGUMENT);
    }
    if (bufsize > 1048576 * 64)  //一次最大读 64M
        bufsize = 1048576 * 64;

    int extent_idx = offset / EXTENT_SIZE;
    uint64_t extent_offset = offset - EXTENT_SIZE * extent_idx;
    uint64_t pos = offset;
    uint32_t TotalReadLen = 0;
    uint32_t OnceReadLen = 0;
    uint32_t ReadedLen = 0;
    cyprestore::common::pb::ExtentRouter router;
    std::string crc32;
    ::cyprestore::common::pb::EsInstance esinstance;
    RpcClientEM rpcClientEM;
    if (rpcClientEM.Init(ExtentManagerIp, ExtentManagerPort) != STATUS_OK)
        return (STATUS_FAILED_PRECONDITION);

    RpcClientES rpcClientES;
    // unsigned char TempBuf[BLOCK_SIZE] = {0};
    std::unique_ptr<char[]> TempBuf;
    int OnceReadLen_yu = 0;
    int extent_offset_yu = 0;

    while (TotalReadLen < bufsize) {
        OnceReadLen = EXTENT_SIZE - extent_offset;
        if (OnceReadLen > bufsize - TotalReadLen)
            OnceReadLen = bufsize - TotalReadLen;

        StatusCode ret =
                rpcClientEM.QueryRouter(blob_id, extent_idx, router, errmsg);
        if (ret != STATUS_OK) {
            bufsize = TotalReadLen;
            return (ret);
        }

        esinstance = router.primary();
        if (rpcClientES.Init(esinstance.public_ip(), esinstance.public_port())
            != STATUS_OK)
            return (STATUS_FAILED_PRECONDITION);

        extent_offset_yu = extent_offset % BLOCK_SIZE;
        OnceReadLen_yu = OnceReadLen % BLOCK_SIZE;
        if (extent_offset_yu == 0 && OnceReadLen_yu == 0)
            ret = rpcClientES.Read(
                    router.extent_id(), extent_offset, buf + TotalReadLen,
                    OnceReadLen, ReadedLen, crc32, errmsg);
        else {
            uint32_t curoffset = extent_offset - extent_offset_yu;
            uint32_t curreadlen = OnceReadLen + extent_offset_yu;
            if (curreadlen % BLOCK_SIZE != 0)
                curreadlen = curreadlen + BLOCK_SIZE - curreadlen % BLOCK_SIZE;

            TempBuf.reset(new char[curreadlen]);
            ret = rpcClientES.Read(
                    router.extent_id(), curoffset, TempBuf.get(), curreadlen,
                    ReadedLen, crc32, errmsg);
            if (ret != STATUS_OK) return (ret);

            memcpy(buf + TotalReadLen, TempBuf.get() + extent_offset_yu,
                   OnceReadLen);
        }

        if (ret != STATUS_OK) {
            bufsize = TotalReadLen;
            return (ret);
        }

        extent_idx++;
        extent_offset = 0;
        TotalReadLen += OnceReadLen;
    }

    retreadlen = TotalReadLen;
    return (STATUS_OK);
}

class OnRPCDoneAsyncWriteBlob : public google::protobuf::Closure {
public:
    void Run() {
        std::unique_ptr<OnRPCDoneAsyncWriteBlob> self_guard(this);

        if (cntl.Failed()) {
            // RPC失败了. response里的值是未定义的，勿用。
            LOG(WARNING) << "OnRPCDoneAsyncWriteExtent rpc Failed "
                         << cntl.ErrorText();
            retcode = STATUS_INTERNAL;
        } else {
            // RPC成功了，response里有我们想要的数据。开始RPC的后续处理。
            retcode = response.status().code();
            if (response.status().code()
                != ::cyprestore::common::pb::STATUS_OK) {
                LOG(INFO) << "OnRPCDoneAsyncWriteExtent failed status.code="
                          << response.status().code()
                          << ", status.message=" << response.status().message();
            }
        }
    }

    brpc::CallId cid;
    cyprestore::extentserver::pb::WriteResponse response;
    brpc::Controller cntl;
    StatusCode retcode;
};

class OnRPCDoneAsyncWriteExtent : public google::protobuf::Closure {
public:
    void Run() {
        std::unique_ptr<OnRPCDoneAsyncWriteExtent> self_guard(this);

        if (cntl.Failed()) {
            // RPC失败了. response里的值是未定义的，勿用。
            LOG(WARNING) << "OnRPCDoneAsyncWriteExtent rpc Failed "
                         << cntl.ErrorText();
            retcode = STATUS_INTERNAL;
        } else {
            // RPC成功了，response里有我们想要的数据。开始RPC的后续处理。
            retcode = response.status().code();
            if (response.status().code()
                != ::cyprestore::common::pb::STATUS_OK) {
                LOG(INFO) << "OnRPCDoneAsyncWriteExtent failed status.code="
                          << response.status().code()
                          << ", status.message=" << response.status().message();
            }
        }
    }

    brpc::CallId cid;
    cyprestore::extentserver::pb::WriteResponse response;
    brpc::Controller cntl;
    StatusCode retcode;
};

StatusCode AsyncWriteExtent(
        const std::string &extent_server_addr, int extent_server_port,
        const std::string &extent_id, uint64_t extent_offset,
        unsigned char *buf, uint32_t bufsize, int timeout_ms) {
    brpc::Channel channel;
    StatusCode ret =
            InitBrpcChannel(channel, extent_server_addr, extent_server_port);
    if (ret != STATUS_OK) return (ret);

    unsigned long BeginTick = GetTickCount();
    cyprestore::extentserver::pb::ExtentService_Stub stub(&channel);

    cyprestore::extentserver::pb::WriteRequest request;
    request.set_extent_id(extent_id);
    request.set_offset(extent_offset);
    request.set_size(bufsize);

    OnRPCDoneAsyncWriteExtent *done = new OnRPCDoneAsyncWriteExtent;
    done->cntl.set_timeout_ms(timeout_ms);
    if (done->cntl.response_attachment().append(buf, bufsize) == -1) {
        LOG(ERROR) << "WriteExtent response_attachment().append failed "
                   << done->cntl.ErrorText();
        return (STATUS_INTERNAL);
    }

    stub.Write(&done->cntl, &request, &done->response, done);
    // done->cid = done->cntl.call_id();
    LOG(INFO) << "AsyncWriteExtent cid " << done->cntl.call_id();
}

};  // namespace cyprestore
