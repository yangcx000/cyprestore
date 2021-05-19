/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "cyprebench.h"

#include <bvar/bvar.h>
#include <signal.h>

#include "butil/logging.h"
#include "common/error_code.h"
#include "utils/chrono.h"

namespace cyprestore {
namespace clients {

bvar::LatencyRecorder g_latency_cypre_bench_write("cypre_bench_write");
bvar::LatencyRecorder g_latency_cypre_bench_read("cypre_bench_read");
bvar::LatencyRecorder g_latency_cypre_bench_submit("cypre_bench_submit");

Cyprebench &Cyprebench::GlobalCyprebench() {
    static Cyprebench globalCyprebench;
    return globalCyprebench;
}

int Cyprebench::Init(const CyprebenchOptions &options) {
    options_ = options;
    if (options_.block_size % kAlignSize != 0) {
        LOG(ERROR) << "Invalid block_size, must be align with " << kAlignSize
                   << ", block_size:" << options_.block_size;
        return -1;
    }

    CypreRBDOptions opt(options_.em_ip, options_.em_port);
    opt.brpc_sender_ring_power = options_.brpc_sender_ring_power;
    opt.brpc_sender_thread = options_.brpc_sender_thread_num;
    opt.brpc_worker_thread_num = options_.brpc_worker_thread_num;
    opt.brpc_sender_thread_cpu_affinity =
            options_.brpc_sender_thread_cpu_affinity;
    opt.proto = options_.nullio ? kNull : kBrpc;

    cypre_rbd_ = CypreRBD::New();
    if (cypre_rbd_->Init(opt) != 0) {
        LOG(ERROR) << "Couldn't init cypre rbd"
                   << ", em_ip:" << options_.em_ip
                   << ", em_port:" << options_.em_port;
        return -1;
    }

    RBDStreamHandlePtr handle;
    int ret = cypre_rbd_->Open(options_.blob_id, handle);
    if (ret < 0) {
        LOG(ERROR) << "Couldn't open blob"
                   << ", blob_id:" << options_.blob_id << ", err_code:" << ret;
        return -1;
    }

    uint64_t blob_size = handle->GetDeviceSize();
    if (options_.size > blob_size) {
        LOG(ERROR) << "Size larger than blob size"
                   << ", size:" << options_.size << ", blob_size:" << blob_size;
        return -1;
    }

    if (options_.size == 0) {
        options_.size = blob_size;
    }

    if (options_.verify) {
        std::string file_path = "./" + handle->GetDeviceId();
        file_ = new File(blob_size, file_path);
        if (file_->Open() != 0) {
            LOG(ERROR) << "Couldn't open local file"
                       << ", file_path:" << file_path;
            return -1;
        }
    }

    cypre_rbd_->Close(handle);
    engine_factory_ = new EngineFactory(options_);
    return 0;
}

static void read_cb(int rc, void *arg) {
    IOContext *io_ctx = static_cast<IOContext *>(arg);
    Cyprebench *cypre_bench = io_ctx->cypre_bench;
    if (rc != 0) {
        LOG(ERROR) << "Couldn't read at offset " << io_ctx->io_u->offset;
        io_ctx->io_depth->fetch_sub(1);
        cypre_bench->PutIoContext(io_ctx);
        return;
    }

    if (utils::Chrono::GetTime(&io_ctx->end_time) != 0) {
        LOG(ERROR) << "Couldn't get end_time";
        io_ctx->io_depth->fetch_sub(1);
        cypre_bench->PutIoContext(io_ctx);
        return;
    }

    g_latency_cypre_bench_read << utils::Chrono::TimeSinceUs(
            &io_ctx->start_time, &io_ctx->end_time);
    io_ctx->io_depth->fetch_sub(1);
    if (cypre_bench->Options()->verify) {
        std::string output(io_ctx->io_u->len, '\0');
        int ret = cypre_bench->GetFile()->Read(
                io_ctx->io_u->offset, io_ctx->io_u->len, output);
        if (ret != 0) {
            LOG(ERROR) << "Couldn't read file"
                       << ", offset:" << io_ctx->io_u->offset
                       << ", len:" << io_ctx->io_u->len;
        }
        if (memcmp(output.data(), io_ctx->io_u->data, io_ctx->io_u->len) != 0) {
            LOG(ERROR) << "Remote and Local data inconsistent"
                       << ", offset:" << io_ctx->io_u->offset
                       << ", len:" << io_ctx->io_u->len;
        }
    }
    cypre_bench->PutIoContext(io_ctx);
}

void Cyprebench::doRead(RBDStreamHandlePtr handle) {
    pthread_t pid = pthread_self();
    engine_factory_->AddEngine(pid, "read");
    engine_factory_->WaitEnginesReady(options_.read_jobs + options_.write_jobs);

    LOG(INFO) << "Read job " << std::hex << pid << " start";

    struct timespec submit_time;
    uint64_t count = options_.io_nums != 0
                             ? options_.io_nums
                             : options_.size / options_.block_size;
    std::atomic<int> io_depth(0);
    while ((!stop_.load(std::memory_order_relaxed) && (count > 0))
           || options_.run_forever) {
        if (io_depth.load() >= options_.io_depth) {
            continue;
        }
        IOContext *io_ctx = GetIoContext(pid, &io_depth);
        if (utils::Chrono::GetTime(&io_ctx->start_time) != 0) {
            LOG(ERROR) << "Couldn't get start_time";
            PutIoContext(io_ctx);
            return;
        }

        int rv = handle->AsyncRead(
                io_ctx->io_u->data, io_ctx->io_u->len, io_ctx->io_u->offset,
                read_cb, io_ctx);
        if (rv != common::CYPRE_OK) {
            LOG(ERROR) << "Couldn't send read at offset "
                       << io_ctx->io_u->offset << ", err_code:" << rv;
            PutIoContext(io_ctx);
            return;
        }

        if (utils::Chrono::GetTime(&submit_time) == 0) {
            g_latency_cypre_bench_submit << utils::Chrono::TimeSinceUs(
                    &io_ctx->start_time, &submit_time);
        }

        io_depth.fetch_add(1);
		if (!options_.run_forever) {
        	--count;
		}
    }

    while (io_depth.load() > 0) {
        usleep(200);
    }

    LOG(INFO) << "Read job " << std::hex << pid << " finished";
}

static void write_cb(int rc, void *arg) {
    IOContext *io_ctx = static_cast<IOContext *>(arg);
    Cyprebench *cypre_bench = io_ctx->cypre_bench;
    if (rc != 0) {
        LOG(ERROR) << "Couldn't write at offset " << io_ctx->io_u->offset;
        io_ctx->io_depth->fetch_sub(1);
        cypre_bench->PutIoContext(io_ctx);
        return;
    }

    if (utils::Chrono::GetTime(&io_ctx->end_time) != 0) {
        LOG(ERROR) << "Couldn't gettime";
        cypre_bench->PutIoContext(io_ctx);
        return;
    }

    g_latency_cypre_bench_write << utils::Chrono::TimeSinceUs(
            &io_ctx->start_time, &io_ctx->end_time);
    io_ctx->io_depth->fetch_sub(1);
    if (cypre_bench->Options()->verify) {
        int ret = cypre_bench->GetFile()->Write(
                io_ctx->io_u->offset, io_ctx->io_u->len, io_ctx->io_u->data);
        if (ret != 0) {
            LOG(ERROR) << "Couldn't write file"
                       << ", offset:" << io_ctx->io_u->offset
                       << ", len:" << io_ctx->io_u->len;
        }
    }
    cypre_bench->PutIoContext(io_ctx);
}

void Cyprebench::doWrite(RBDStreamHandlePtr handle) {
    pthread_t pid = pthread_self();
    engine_factory_->AddEngine(pid, "write");
    engine_factory_->WaitEnginesReady(options_.read_jobs + options_.write_jobs);

    LOG(INFO) << "Write job " << std::hex << pid << " start";

    struct timespec submit_time;
    uint64_t count = options_.io_nums != 0
                             ? options_.io_nums
                             : options_.size / options_.block_size;
    std::atomic<int> io_depth(0);
    while ((!stop_.load(std::memory_order_relaxed) && (count > 0))
           || options_.run_forever) {
        if (io_depth.load() >= options_.io_depth) {
            continue;
        }

        IOContext *io_ctx = GetIoContext(pid, &io_depth);
        if (utils::Chrono::GetTime(&io_ctx->start_time) != 0) {
            LOG(ERROR) << "Couldn't gettime";
            PutIoContext(io_ctx);
            return;
        }

        int rv = handle->AsyncWrite(
                io_ctx->io_u->data, io_ctx->io_u->len, io_ctx->io_u->offset,
                write_cb, io_ctx);
        if (rv != common::CYPRE_OK) {
            LOG(ERROR) << "Couldn't send write at offset "
                       << io_ctx->io_u->offset << ", ret:" << rv;
            PutIoContext(io_ctx);
            return;
        }
        if (utils::Chrono::GetTime(&submit_time) == 0) {
            g_latency_cypre_bench_submit << utils::Chrono::TimeSinceUs(
                    &io_ctx->start_time, &submit_time);
        }

        io_depth.fetch_add(1);
		if (!options_.run_forever) {
        	--count;
		}
    }

    while (io_depth.load() > 0) {
        usleep(200);
    }

    LOG(INFO) << "Write job " << std::hex << pid << " finished";
}

IOContext *Cyprebench::GetIoContext(pthread_t pid, std::atomic<int> *io_depth) {
    IOContext *io_ctx;
    auto s = engine_factory_->ioctx_pools_[pid]->Dequeue((void **)&io_ctx);
    if (!s.ok()) {
        io_ctx = new IOContext();
    }
    io_ctx->cypre_bench = this;
    io_ctx->io_depth = io_depth;
    io_ctx->io_u = engine_factory_->GetIO(pid);
    io_ctx->pid = pid;
    return io_ctx;
}

void Cyprebench::PutIoContext(IOContext *io_ctx) {
	pthread_t pid = io_ctx->pid;
    engine_factory_->PutIO(pid, io_ctx->io_u);
    auto s = engine_factory_->ioctx_pools_[pid]->Enqueue(io_ctx);
    if (!s.ok()) {
        LOG(ERROR) << "Couldn't put ioctx to pool"
                   << ", err:" << s.ToString();
    }
}

void *Cyprebench::bootstrapRead(void *arg) {
    Cyprebench *bench = static_cast<Cyprebench *>(arg);
    RBDStreamHandlePtr handle;
    int rv = bench->cypre_rbd_->Open(bench->options_.blob_id, handle);
    if (rv < 0) {
        LOG(ERROR) << "Couldn't open blob"
                   << ", blob_id:" << bench->options_.blob_id
                   << ", err_code:" << rv;
        return nullptr;
    }
    bench->doRead(handle);
    bench->cypre_rbd_->Close(handle);
    return nullptr;
}

void *Cyprebench::bootstrapWrite(void *arg) {
    Cyprebench *bench = static_cast<Cyprebench *>(arg);
    RBDStreamHandlePtr handle;
    int rv = bench->cypre_rbd_->Open(bench->options_.blob_id, handle);
    if (rv < 0) {
        LOG(ERROR) << "Couldn't open blob"
                   << ", blob_id:" << bench->options_.blob_id
                   << ", err_code:" << rv;
        return nullptr;
    }
    bench->doWrite(handle);
    bench->cypre_rbd_->Close(handle);
    return nullptr;
}

int Cyprebench::launchThreads(const std::string &rw) {
    int ret = 0;
    std::vector<pthread_t> *tids;
    std::vector<int> coremask;
    bootstrap_func func = nullptr;
    if (rw == "read") {
        read_jobs_.resize(options_.read_jobs);
        tids = &read_jobs_;
        coremask = options_.read_jobs_coremask;
        func = Cyprebench::bootstrapRead;
    } else if (rw == "write") {
        write_jobs_.resize(options_.write_jobs);
        tids = &write_jobs_;
        coremask = options_.write_jobs_coremask;
        func = Cyprebench::bootstrapWrite;
    } else {
        LOG(ERROR) << "Invalid operation " << rw;
        return -1;
    }

    for (size_t i = 0, icpu = 0; i < tids->size(); ++i, ++icpu) {
        ret = pthread_create(&(*tids)[i], nullptr, func, this);
        if (ret != 0) {
            LOG(ERROR) << "Couldn't create " << rw << " job pthread";
            return -1;
        }
        if (coremask.empty()) {
            continue;
        }
        if (icpu >= coremask.size()) {
            icpu = 0;
        }
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(coremask[icpu], &cpuset);
        ret = pthread_setaffinity_np((*tids)[i], sizeof(cpuset), &cpuset);
        if (ret != 0) {
            LOG(ERROR) << "Couldn't set " << rw
                       << " job cpu affinity, ret=" << ret;
            return -1;
        }
    }
    return 0;
}

void Cyprebench::sigHandler(int signum) {
    switch (signum) {
        case SIGTERM:
        case SIGINT:
            Cyprebench::GlobalCyprebench().Stop();
            break;
        default:
            break;
    }
}

void Cyprebench::Stop() {
    stop_.store(true, std::memory_order_relaxed);
    LOG(INFO) << "Stop job threads";
}

void Cyprebench::Run() {
    if (options_.read_jobs > 0) {
        if (launchThreads("read") != 0) {
            LOG(ERROR) << "Couldn't launch read jobs";
            return;
        }
    }

    if (options_.write_jobs > 0) {
        if (launchThreads("write") != 0) {
            LOG(ERROR) << "Couldn't launch write jobs";
            return;
        }
    }

    signal(SIGINT, Cyprebench::sigHandler);
    signal(SIGTERM, Cyprebench::sigHandler);

    for (size_t i = 0; i < read_jobs_.size(); ++i) {
        pthread_join(read_jobs_[i], nullptr);
    }

    for (size_t i = 0; i < write_jobs_.size(); ++i) {
        pthread_join(write_jobs_[i], nullptr);
    }

    LOG(INFO) << "Cyprebench run to completion";
    return;
}

}  // namespace clients
}  // namespace cyprestore
