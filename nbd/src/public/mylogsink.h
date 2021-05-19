/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 *
 */

#ifndef CYPRESTORE_UTILS_MYLOGSINK_H_
#define CYPRESTORE_UTILS_MYLOGSINK_H_

#include <bthread/bthread.h>
#include <butil/logging.h>

#include <cassert>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

namespace cyprestore {
namespace nbd {

class MyLogSink : public logging::LogSink {
public:
    MyLogSink();
    ~MyLogSink();

    bool OnLogMessage(
            int severity, const char *file, int line,
            const butil::StringPiece &content);

    bool
    Init(const std::string &log_path, const std::string &prefix, int SaveDays,
         size_t SplitLogSizeM = 0);
    void Flush(int severity);
    void Shutdown();

private:
    std::mutex mutex_;
    // spin_lock spin_lock_;

    std::string LogPath_;
    std::string prefix_;
    int SaveDays_;
    std::string ModuleName_;

    std::string ErrorLogFileName_;
    std::string InfoLogFileName_;

    size_t ErrorLogFileLen_;
    size_t InfoLogFileLen_;
    size_t SplitLogSize_;  //日志文件切分大小
    std::string CurDay_;

    FILE *ErrorLogFile_;
    FILE *InfoLogFile_;
    time_t ErrorLastFlushTime_;
    time_t InfoLastFlushTime_;

    void WriteLogFile(
            int severity, const char *Buf, int Len, bool bflush,
            const std::string &NewDay);

    static const int CacheSize_ = 1048576;
    char InfoCacheBuf[CacheSize_];
    int InfoCachePos_;

    char ErrorCacheBuf[CacheSize_];
    int ErrorCachePos_;

    // bthread_t flush_th;
    std::thread flushThread_;

public:
    bool terminated_;
};

}  // namespace nbd
}  // namespace cyprestore

/*使用
::cyprestore::utils::MyLogSink logSink(log_path);
::logging::LogSink* old_sink = ::logging::SetLogSink(&logSink);
*/

#endif  // CYPRESTORE_UTILS_MYLOGSINK_H_
