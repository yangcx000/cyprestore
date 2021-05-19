/*
 * Copyright 2020 JDD authors.
 * @feifei5
 *
 */

#include "gtest/gtest.h"

#include <iostream>
#include <string>

#include "butil/iobuf.h"
#include "utils/chrono.h"
#include "utils/crc32.h"

namespace cyprestore {
namespace utils {
namespace {

class Crc32Test : public ::testing::Test {
 protected:
	void SetUp() override {
        str = std::string(size_, 'a');
	}

	void TearDown() override {
	}

    std::string str;
    int size_ = 4096;
};

TEST_F(Crc32Test, TestChecksum) {
    auto crc32 = Crc32::Checksum(str);

    ASSERT_EQ(crc32, Crc32::Checksum(str.c_str(), size_));

    butil::IOBuf buf;
    buf.append(str.c_str(), size_);
    ASSERT_EQ(crc32, Crc32::Checksum(buf));
}

// 4K: 0.7us per crc32
// 8k: 1.37us per crc32
// 16k: 2.73us per crc32
TEST_F(Crc32Test, TestPerf) {
    butil::IOBuf buf;
    buf.append(str.c_str(), size_);
    int count = 5000000;
    timespec begin, end;
    Chrono::GetTime(&begin);
    while (count > 0) {
        count--;
        Crc32::Checksum(buf);
    }
    Chrono::GetTime(&end);
    std::cout << "Run checksum cost " << Chrono::TimeSinceUs(&begin, &end) << std::endl;
}

} // namespace
} // namespace utils
} // namespace cyprestore
