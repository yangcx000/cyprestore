/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include <bvar/bvar.h>
#include <brpc/server.h>
#include <butil/time.h>
#include <chrono>
#include <gflags/gflags.h>
#include <iomanip> // setw
#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <thread>

#include "extentserver/bitmap_allocator.h"
#include "extentserver/io_mem.h"
#include "extentserver/spdk_mgr.h"
#include "common/config.h"
#include "common/log.h"

DEFINE_string(conf, "", "INI config file of ExtentServer");
DEFINE_int32(jobs, 1, "Number of jobs");

bvar::LatencyRecorder g_latency_write("spdk_write");
bvar::LatencyRecorder g_latency_read("spdk_read");
bvar::LatencyRecorder g_latency_bitmap_alloc("bitmap_allocd");

const size_t AllocMemSize = 1 << 30;
const size_t AlignSize = 4 << 10;

static void Usage() {
	std::cout << "Usage:" << std::endl
			  << std::setw(30) << "spdk_test -conf=xxxx -jobs=1" << std::endl;
}

using namespace cyprestore::extentserver;
using namespace cyprestore::common;

IOMemMgr g_iomem_mgr(16);
SpdkMgr *g_spdk_mgr = nullptr;
BitmapAllocator g_bitmap_allocator;

static SpdkMgr *init_spdk_mgr() {
	const cyprestore::common::SpdkCfg &spdk_cfg = cyprestore::GlobalConfig().spdk();	
	SpdkEnvOptions env_options;
	env_options.shm_id = spdk_cfg.shm_id;
	env_options.mem_channel = spdk_cfg.mem_channel;
	env_options.mem_size = spdk_cfg.mem_size;
	env_options.master_core = spdk_cfg.master_core;
	env_options.num_pci_addr = spdk_cfg.num_pci_addr;
	env_options.no_pci = spdk_cfg.no_pci;
	env_options.hugepage_single_segments = spdk_cfg.hugepage_single_segments;
	env_options.unlink_hugepage = spdk_cfg.unlink_hugepage;
	env_options.core_mask = spdk_cfg.core_mask;
	env_options.huge_dir = spdk_cfg.huge_dir;
	env_options.name = spdk_cfg.name;
    env_options.json_config_file  = spdk_cfg.json_config_file;
    env_options.rpc_addr = spdk_cfg.rpc_addr;

	g_spdk_mgr = new SpdkMgr(env_options);
	return g_spdk_mgr;
}

static void dump_dev_info() {
	std::cout << "block_size:" << g_spdk_mgr->GetBlockSize()
			  << "\nnum_blocks:" << g_spdk_mgr->GetNumBlocks()
			  << "\nwrite_unit_size:" << g_spdk_mgr->GetWriteUnitSize()
 			  << "\nbuf_align_size:" << g_spdk_mgr->GetBufAlignSize()	
 			  << "\nmd_size:" << g_spdk_mgr->GetMDSize()
 			  << "\noptimal_io_boundary:" << g_spdk_mgr->GetOptimalIOBoundary()
 			  << "\nWriteCacheEnabled:" << g_spdk_mgr->WriteCacheEnabled() << std::endl;
}

const uint64_t kWriteRange = 100;

static void *write_func(void *arg) {
	int index = *static_cast<int *>(arg);
	std::cout << "Launch write thread " << index << std::endl;

	int iters = (AllocMemSize * kWriteRange) / AlignSize;
	uint64_t offset = index * kWriteRange * AllocMemSize;	
	std::string data(AlignSize, 'a' + index);
	Status s;
	butil::Timer timer;
	for (int i = 0; i < iters; ++i) {
		timer.start();

		io_u *io = g_iomem_mgr.GetIOUnit(AlignSize);	
		memcpy(io->data, data.c_str(), AlignSize);

		s = g_spdk_mgr->Write(io->data, offset + i * AlignSize, AlignSize);
		//s = g_spdk_mgr->WriteZeroes(offset + i * AlignSize, AlignSize);
		if (!s.ok()) {
			std::cout << s.ToString() << std::endl;
			return nullptr;
		}
		g_iomem_mgr.PutIOUnit(io);	

		timer.stop();
		g_latency_write << timer.u_elapsed();
		//std::cout << "Thread-" << index <<  " write offset " << offset + i * AlignSize << std::endl;
	}

	std::cout << "Finish write thread " << index << std::endl;
	return nullptr;
}

static void *read_func(void *arg) {
	int index = *static_cast<int *>(arg);
	std::cout << "Launch read thread " << index << std::endl;
	
	int iters = (AllocMemSize * kWriteRange) / AlignSize;
	uint64_t offset = index * kWriteRange * AllocMemSize;	
	std::string data(AlignSize, 'a' + index);
	Status s;
	butil::Timer timer;

	for (int i = 0; i < iters; ++i) {
		timer.start();

		io_u *io = g_iomem_mgr.GetIOUnit(AlignSize);	
		s = g_spdk_mgr->Read(io->data, offset + i * AlignSize, AlignSize);
		if (!s.ok()) {
			std::cout << s.ToString() << std::endl;
			return nullptr;
		}
		g_latency_read << timer.u_elapsed();
		if (memcmp(io->data, data.c_str(), AlignSize) != 0) {
			std::cerr << "read data inconsistence" << std::endl;
		}
		
		g_iomem_mgr.PutIOUnit(io);	
		timer.stop();
	}

	std::cout << "Finish read thread " << index << std::endl;
	return nullptr;
}

static void test_write() {
	pthread_t tids[FLAGS_jobs];
	for (int i = 0; i < FLAGS_jobs; ++i) {
		int *arg = new int(i);
		int ret = pthread_create(&tids[i], NULL, write_func, arg);
		if (ret != 0) return;
	}

	for (int i = 0; i < FLAGS_jobs; ++i) {
		pthread_join(tids[i], nullptr);
	}
}

static void test_read() {
	pthread_t tids[FLAGS_jobs];
	for (int i = 0; i < FLAGS_jobs; ++i) {
		int *arg = new int(i);
		int ret = pthread_create(&tids[i], NULL, read_func, arg);
		if (ret != 0) return;
	}

	for (int i = 0; i < FLAGS_jobs; ++i) {
		pthread_join(tids[i], nullptr);
	}
}

void *alloc_bitmap(void *arg) {
	butil::Timer timer;
	while (true) {
		timer.start();
		AUnit aunit;
		Status s = g_bitmap_allocator.Allocate(4096, &aunit);
		if (!s.ok()) {
			std::cout << s.ToString() << std::endl;
			return nullptr;
		}
		timer.stop();
		g_latency_bitmap_alloc << timer.u_elapsed();	
	}
}

static void test_bitmap() {
	pthread_t tids[FLAGS_jobs];
	for (int i = 0; i < FLAGS_jobs; ++i) {
		int *arg = new int(i);
		int ret = pthread_create(&tids[i], NULL, alloc_bitmap, arg);
		if (ret != 0) return;
	}

	for (int i = 0; i < FLAGS_jobs; ++i) {
		pthread_join(tids[i], nullptr);
	}
}

static std::ostream& operator<<(std::ostream &os, const SpdkEnvOptions &options) {
	os << std::setfill('-') << std::setw(40) << '-' << "[SPDK Env Options]" << std::setfill('-') << std::setw(40) << '-'
	   << "\nshm_id:" << options.shm_id
	   << "\nmem_channel:" << options.mem_channel
	   << "\nmem_size:" << options.mem_size
	   << "\nmaster_core:" << options.master_core
	   << "\nnum_pci_addr:" << options.num_pci_addr
	   << "\nno_pci:" << options.no_pci
	   << "\nhugepage_single_segments:" << options.hugepage_single_segments
	   << "\nunlink_hugepage:" << options.unlink_hugepage
	   << "\ncore_mask:" << options.core_mask
	   << "\nhuge_dir:" << options.huge_dir
	   << "\nname:" << options.name
	   << "\njson_config_file:" << options.json_config_file;
	return os;
}

int main(int argc, char **argv) {
    GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);
    if (FLAGS_conf.empty()) {
		Usage();
        return -1;
    }

	if (cyprestore::GlobalConfig().Init("extentserver", FLAGS_conf) != 0) { return -1; }
	if (init_spdk_mgr() == nullptr) return -1;	
	
	std::cout << g_spdk_mgr->GetEnvOptions() << std::endl;	

	Status s = g_spdk_mgr->InitEnv();
	if (!s.ok()) {
		std::cout << s.ToString() << std::endl;
		return -1;
	}

	s = g_spdk_mgr->Open("Nvme0n1");
	if (!s.ok()) {
		std::cout << s.ToString() << std::endl;
		return -1;
	}
	dump_dev_info();			

	brpc::StartDummyServerAt(8080);	
	/*
	{
		Status s = g_iomem_mgr.Init();
		if (!s.ok()) {
			std::cout << s.ToString() << std::endl;
			return -1;
		}
	}

	// Test Write
	test_write();

	// Test Read
	test_read();
	*/
	//g_bitmap_allocator.Init(4ULL << 10, 4ULL << 40);
	//test_bitmap();

	std::this_thread::sleep_for(std::chrono::seconds(3600));				
	return 0;
}
