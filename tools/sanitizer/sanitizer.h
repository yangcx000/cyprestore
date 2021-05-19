/*
 * Copyright 2020 JDD authors.
 * @yangchunxin3
 *
 */

#include <pthread.h>

#include <atomic>
#include <string>
#include <vector>

namespace cyprestore {
namespace tools {

class Sanitizer;

struct Options {
	Options()
		: num_jobs(0), block_size(0), range_size(0) {}

	std::string device_name;	
	int32_t num_jobs;
	size_t block_size; 
	uint64_t range_size;
};

struct JobContext {
	JobContext(Sanitizer *s, int i)
		: sa(s), index(i) {}  

	Sanitizer *sa;
	int index;
};

class Sanitizer {
 public:
	Sanitizer(const Options &options, bool do_verify = false)
		: options_(options), device_size_(0),
          part_size_(0), target_size_(0), fd_(-1), do_verify_(do_verify), result_(0) {}
	~Sanitizer() = default;

	int Run();
	void SetFailed() { result_.store(-1, std::memory_order_relaxed); }
	void SetDoVerify() { do_verify_ = true; }

 private:
	static void *jobEntry(void *arg);
	int openDevice();
	int getDeviceSize();
	int launchJobs();
	int doJob(int index);
	void waitJobs();

	Options options_;
	uint64_t device_size_;
	uint64_t part_size_;
	uint64_t target_size_;
	int fd_;
	bool do_verify_;
	std::atomic<int> result_;
	std::vector<pthread_t> jobs_;
};

}  // namespace tools
}  // namespace cyprestore
