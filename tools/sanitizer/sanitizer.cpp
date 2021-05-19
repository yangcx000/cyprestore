/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include "sanitizer.h"

#include <butil/logging.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace cyprestore {
namespace tools {

const int kErrorRetrys = 3;

int Sanitizer::openDevice() {
	fd_ = open(options_.device_name.c_str(), O_RDWR | O_SYNC);
	if (fd_ < 0) {
		LOG(ERROR) << "Couldn't open device " << options_.device_name
                   << ", err:" << strerror(errno);
		return -1;
	}
	return 0;
}

int Sanitizer::getDeviceSize() {
	const int kSectorSize = 512;
	long sector_count = 0;

	if (ioctl(fd_, BLKGETSIZE, &sector_count) != 0) {
		LOG(ERROR) << "Couldn't get sector count of device " << options_.device_name
                   << ", err:" << strerror(errno);
		return -1;
	}

	// XXX(yangchunxin3):BLKSSZGET拿到的sector_size为4k, 但是BLKGETSIZE按sector_size为512拿的sector count
	/*
	if (ioctl(fd_, BLKSSZGET, &sector_size) != 0) {
		LOG(ERROR) << "Couldn't get sector size of device " << options_.device_name
                   << ", err:" << strerror(errno);
		return -1;
	}
	*/

	device_size_ = (uint64_t)kSectorSize * sector_count;
	LOG(INFO) << "Get device info, device_name:" << options_.device_name
			  << ", sector_size:" << kSectorSize
              << ", sector_count:" << sector_count
			  << ", device_size:" << device_size_;
	return 0;
}

int Sanitizer::doJob(int index) {
	uint64_t start = index * part_size_;
	if (start >= target_size_) { return 0; }

	int retry_count = 0;
	uint64_t end = (start + part_size_) >= target_size_ ? target_size_ : (start + part_size_);
	while (start < end) {
		size_t len = (end - start) >= options_.block_size ? options_.block_size : (end - start);
		if (do_verify_) {
			std::string data_buf(len, 'x');
			std::string zero_data(len, '\0');
			if (pread(fd_, const_cast<char*>(data_buf.data()), len, start) != (ssize_t)len) {
				if (retry_count < kErrorRetrys) {
					++retry_count;	
					continue;
				}
				LOG(ERROR) << "Couldn't read data, err:" << strerror(errno)
					       << ", offset:" << start << ", len:" << len;
				return -1;
			}
			if (memcmp(data_buf.data(), zero_data.data(), len) != 0) {
				LOG(ERROR) << "Data on disk has nonezero"
					       << ", offset:" << start << ", len:" << len;
				return -1;
			}
		} else {
			std::string data_buf(len, '\0');
			if (pwrite(fd_, data_buf.data(), len, start) != (ssize_t)len) {
				if (retry_count < kErrorRetrys) {
					++retry_count;	
					continue;
				}
				LOG(ERROR) << "Couldn't wipe data zero, err:" << strerror(errno)
					       << ", offset:" << start << ", len:" << len;
				return -1;
			}
		}

		start += len;
		if (retry_count != 0) {
			retry_count = 0;
		}
	}
	return 0;
}

void *Sanitizer::jobEntry(void *arg) {
	JobContext *ctx = static_cast<JobContext *>(arg);	
	Sanitizer *sa = ctx->sa;
	if (sa->doJob(ctx->index) != 0) {
		sa->SetFailed();
	}
	
	delete ctx;
	return nullptr;
}

int Sanitizer::launchJobs() {
	int ret = 0;
	jobs_.resize(options_.num_jobs);
	for (size_t i = 0; i < jobs_.size(); ++i) {
		JobContext *ctx = new JobContext(this, i);	
		ret = pthread_create(&jobs_[i], nullptr, jobEntry, (void *)ctx);
		if (ret != 0) {
			LOG(ERROR) << "Couldn't create job " << i;
			return -1;
		}
	}
	return 0;
}

void Sanitizer::waitJobs() {
	for (size_t i = 0; i < jobs_.size(); ++i) {
		pthread_join(jobs_[i], nullptr);
	}
}

int Sanitizer::Run() {
	if (openDevice() != 0) { return -1; }
	if (getDeviceSize() != 0) { return -1; }

	if (options_.num_jobs <= 0) {
		LOG(ERROR) << "Job number " << options_.num_jobs << " must > 0";
		return -1;
	}

	if (options_.range_size == 0) {
		options_.range_size = device_size_;
	}

	target_size_ = options_.range_size >= device_size_ ? device_size_ : options_.range_size;
	part_size_ = (target_size_ + options_.num_jobs - 1) / options_.num_jobs;
	if (launchJobs() != 0) { return -1; }

	if (do_verify_) {
		LOG(INFO) << "Start to verify device " << options_.device_name;
	} else {
		LOG(INFO) << "Start to sanitize device " << options_.device_name;
	}
	
	waitJobs();
	return result_.load(std::memory_order_relaxed);
}

}  // namespace tools
}  // namespace cyprestore
