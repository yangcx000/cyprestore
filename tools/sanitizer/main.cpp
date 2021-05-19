/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include <gflags/gflags.h>
#include <iostream>

#include "sanitizer.h"

DEFINE_string(device_name, "", "device name, eg. /dev/nvme0n1");
DEFINE_int32(num_jobs, 1, "number of erase/verify jobs");
DEFINE_int32(block_size, 1024 * 1024, "block size of erase/verify operation. unit bytes");
DEFINE_uint64(range_size, 0, "range size of erase/verify operation. unit bytes, [0, range_size]");  // just for test
DEFINE_bool(verify, false, "check device has been zeroed");

static void Usage() {
	std::cout << "Usage: sanitizer"
              << "\n  -device_name=[/dev/nvme0n1]"
              << "\n  -num_jobs=[1]"
              << "\n  -block_size=[4096]"
              << "\n  -range_size=[0]"
              << "\n  -verify=[true|false]" << std::endl;
}

using namespace cyprestore::tools;

int main(int argc, char **argv) {
    GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);
	if (FLAGS_device_name.empty()) {
		Usage();
		exit(-1);
	}

	Options options;
	options.device_name = FLAGS_device_name;
	options.num_jobs = FLAGS_num_jobs;
	options.block_size = static_cast<size_t>(FLAGS_block_size);
	options.range_size = FLAGS_range_size;
	
	{
		Sanitizer sanitizer(options);
		if (sanitizer.Run() != 0) {
			std::cout << "Sanitizer device " << FLAGS_device_name << " failed" << std::endl;
			return -1;
		}

		std::cout << "Sanitizer device " << FLAGS_device_name << " succeed" << std::endl;
	}

	if (FLAGS_verify) {
		// 2X sanitizer
		options.block_size *= 2;
		Sanitizer sanitizer(options, FLAGS_verify);
		if (sanitizer.Run() != 0) {
			std::cout << "Verify device " << FLAGS_device_name << " failed" << std::endl;
			return -1;
		}

		std::cout << "Verify device " << FLAGS_device_name << " succeed" << std::endl;
	}
	return 0;	
}
