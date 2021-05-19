#include "utils/timer_thread.h"

#include <chrono>
#include <iostream>
#include <thread>

#include "gtest/gtest.h"

namespace cyprestore {
namespace utils {
namespace {

class TimerThreadTest : public ::testing::Test {
protected:
    void SetUp() override {
        tt_ = GetOrCreateGlobalTimerThread();
    }

    void TearDown() override {}

    static void TimerFunc1(void *arg) {
        // EXPECT_TRUE(false) << static_cast<char *>(arg);
    }

    static void TimerFunc2(void *arg) {
        // EXPECT_TRUE(false) << static_cast<char *>(arg);
    }

    TimerThread *tt_;
};

TEST_F(TimerThreadTest, TestCreateTimer) {
    ASSERT_TRUE(tt_ != nullptr);

    TimerOptions to;
    to.repeated = true;
    to.interval_milliseconds = 2000;
    to.func = TimerFunc1;
    char *msg = const_cast<char *>("repeated timer!");
    to.arg = static_cast<void *>(msg);
    EXPECT_EQ(tt_->create_timer(to), 0);

    EXPECT_EQ(tt_->timer_sizes(), 1);
    std::this_thread::sleep_for(std::chrono::seconds(5));
}

TEST_F(TimerThreadTest, TestTimerSizes) {
    ASSERT_TRUE(tt_ != nullptr);
    EXPECT_EQ(tt_->timer_sizes(), 1);
}

}  // namespace
}  // namespace utils
}  // namespace cyprestore
