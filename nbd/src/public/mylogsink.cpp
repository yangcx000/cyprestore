/*
 * Copyright 2020 JDD authors.
 * @yangbing1
 *
 */

#include "mylogsink.h"

#include <brpc/controller.h>
#include <butil/logging.h>

#include <iomanip>
#include <thread>

#include "plus.h"

namespace cyprestore {
namespace nbd {

static void *ThreadFlushLog(void *arg);

MyLogSink::MyLogSink() {
    ErrorLogFile_ = NULL;
    InfoLogFile_ = NULL;
    GetModuleFileName(false, ModuleName_);
    ErrorLastFlushTime_ = 0;
    InfoLastFlushTime_ = 0;
    ErrorLogFileLen_ = 0;
    InfoLogFileLen_ = 0;
    SplitLogSize_ = 0;

    memset(InfoCacheBuf, 0, CacheSize_);
    InfoCachePos_ = 0;
    memset(ErrorCacheBuf, 0, CacheSize_);
    ErrorCachePos_ = 0;
    terminated_ = false;
}

MyLogSink::~MyLogSink() {

    Shutdown();

    if (InfoLogFile_ != NULL) fclose(InfoLogFile_);
    if (ErrorLogFile_ != NULL) fclose(ErrorLogFile_);
}
void MyLogSink::Shutdown() {
    terminated_ = true;
    if (flushThread_.joinable())
        flushThread_.join();
}

bool MyLogSink::Init(
        const std::string &logPath, const std::string &prefix, int saveDays,
        size_t splitLogSizeM) {
    LogPath_ = CheckPath(logPath);
    if (!ForceDirStrong(LogPath_.c_str())) {
        std::cerr << "Fail to create path " << LogPath_;
        return (false);

        flushThread_ = std::thread(&ThreadFlushLog, this);
    }

    prefix_ = prefix;
    SaveDays_ = saveDays;
    SplitLogSize_ = splitLogSizeM * 1048576;

    return (true);
}

bool MyLogSink::OnLogMessage(
        int severity, const char *file, int line,
        const butil::StringPiece &content) {
    // printf("MyLogSink::OnLogMessage severity=%d\r\n",severity);
    timeval tv;
    gettimeofday(&tv, NULL);
    time_t t = tv.tv_sec;
    struct tm local_tm = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL };
    localtime_r(&t, &local_tm);

    std::ostringstream os;
    const char prev_fill = os.fill('0');
    os << std::setw(2) << local_tm.tm_hour << ':' << std::setw(2)
       << local_tm.tm_min << ':' << std::setw(2) << local_tm.tm_sec << ' ';
    os.fill(prev_fill);

    bool bflush = false;
    if (severity >= logging::BLOG_ERROR) {
        os << "ERR " << file << ':' << line << "] ";
        if (ErrorLastFlushTime_ < t - 2) {
            ErrorLastFlushTime_ = t;
            bflush = true;
        }
    } else {
        if (InfoLastFlushTime_ < t - 2) {
            InfoLastFlushTime_ = t;
            bflush = true;
        }
    }

    os << content << '\n';  //
    std::string log = os.str();
    if (severity >= logging::BLOG_ERROR) {
        fwrite(log.data(), log.size(), 1, stderr);
        fflush(stderr);
    } else {
        fwrite(log.data(), log.size(), 1, stdout);
        fflush(stdout);
    }

    int day = (local_tm.tm_year + 1900) * 10000 + (local_tm.tm_mon + 1) * 100
              + local_tm.tm_mday;
    WriteLogFile(severity, log.data(), log.size(), bflush, std::to_string(day));
    return true;
}

void MyLogSink::WriteLogFile(
        int severity, const char *Buf, int Len, bool bflush,
        const std::string &NewDay) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string LogFileExt, CurLogFileName, NewLogFileName;
    int LogIndex = 0;
    std::string LogIndexStr;

    FILE *LogFileHandle = NULL;
    if (severity >= logging::BLOG_ERROR) {
        CurLogFileName = ErrorLogFileName_;
        LogFileHandle = ErrorLogFile_;
        if (CurDay_ != NewDay) ErrorLogFileLen_ = 0;

        if (SplitLogSize_ > 0) {
            LogIndex = (ErrorLogFileLen_ + SplitLogSize_ - 1) / SplitLogSize_;
            if (LogIndex > 0) LogIndexStr = std::to_string(LogIndex);
        }

        NewLogFileName = LogPath_ + NewDay + "_" + ModuleName_ + prefix_
                         + "_error" + LogIndexStr + ".log";
    } else {

        CurLogFileName = InfoLogFileName_;
        LogFileHandle = InfoLogFile_;
        if (CurDay_ != NewDay) InfoLogFileLen_ = 0;

        if (SplitLogSize_ > 0) {
            LogIndex = (InfoLogFileLen_ + SplitLogSize_ - 1) / SplitLogSize_;
            if (LogIndex > 0) LogIndexStr = std::to_string(LogIndex);
        }

        NewLogFileName = LogPath_ + NewDay + "_" + ModuleName_ + prefix_
                         + "_info" + LogIndexStr + ".log";
    }

    if (LogFileHandle == NULL || NewLogFileName != CurLogFileName) {

        std::vector<std::string> filearr;
        GetFileList(LogPath_ + "*.log", filearr);
        std::sort(filearr.begin(), filearr.end());
        int DelCount = filearr.size() - SaveDays_;
        std::string msg;
        if (DelCount > 0)
            for (int i = 0; i < DelCount; i++) {
                DeleteFile((LogPath_ + filearr[i]).c_str());
            }

        if (severity >= logging::BLOG_ERROR) {
            ErrorLogFileName_ = NewLogFileName;
            if (ErrorLogFile_ != NULL) fclose(ErrorLogFile_);
            ErrorLogFile_ = fopen(NewLogFileName.c_str(), "a");
            LogFileHandle = ErrorLogFile_;
            ErrorLogFileName_ = NewLogFileName;
        } else {
            InfoLogFileName_ = NewLogFileName;
            if (InfoLogFile_ != NULL) fclose(InfoLogFile_);
            InfoLogFile_ = fopen(NewLogFileName.c_str(), "a");
            LogFileHandle = InfoLogFile_;
            InfoLogFileName_ = NewLogFileName;
        }

        if (LogFileHandle == NULL) {
            fprintf(stderr, "Fail to fopen %s , error:%s\r\n",
                    NewLogFileName.c_str(), strerror(errno));
            return;
        }

        CurDay_ = NewDay;
    }

    if (severity >= logging::BLOG_ERROR) {
        if (ErrorCachePos_ + Len > CacheSize_) return;
        memcpy(ErrorCacheBuf, Buf, Len);
        ErrorCachePos_ += Len;
    } else {
        if (InfoCachePos_ + Len > CacheSize_) return;
        memcpy(InfoCacheBuf, Buf, Len);
        InfoCachePos_ += Len;
    }
}

void MyLogSink::Flush(int severity) {

    std::lock_guard<std::mutex> lock(mutex_);

    if (severity >= logging::BLOG_ERROR) {
        if (ErrorLogFile_ == NULL || ErrorCachePos_ <= 0) {
            return;
        }

        size_t WriteCount =
                fwrite(ErrorCacheBuf, ErrorCachePos_, 1, ErrorLogFile_);

        ErrorCachePos_ = 0;
        if (WriteCount <= 0) {
            fclose(ErrorLogFile_);
            ErrorLogFile_ = NULL;
            std::cerr << "Fail to fwrite ErrorLogFile " << ErrorLogFileName_;
        } else {
            ErrorLogFileLen_ += ErrorCachePos_;
            fflush(ErrorLogFile_);
        }

    } else {
        if (InfoLogFile_ == NULL || InfoCachePos_ <= 0) {
            return;
        }

        size_t WriteCount =
                fwrite(InfoCacheBuf, InfoCachePos_, 1, InfoLogFile_);
        InfoCachePos_ = 0;
        if (WriteCount <= 0) {
            fclose(InfoLogFile_);
            InfoLogFile_ = NULL;
            std::cerr << "Fail to fwrite InfoLogFile " << InfoLogFileName_;
        } else {
            InfoLogFileLen_ += InfoCachePos_;
            fflush(InfoLogFile_);
        }
    }
}

static void *ThreadFlushLog(void *arg) {

    MyLogSink *myLogSink = (MyLogSink *)arg;
    int i = 0;

    while (!myLogSink->terminated_ && !brpc::IsAskedToQuit()) {
        i++;
        if (i > 10000) i = 0;
        myLogSink->Flush(i % 2 ? logging::BLOG_ERROR : logging::BLOG_INFO);

        // bthread_usleep(5000); //microseconds
        std::chrono::milliseconds timespan(500);
        std::this_thread::sleep_for(timespan);
    }
    myLogSink->Flush(logging::BLOG_ERROR);
    myLogSink->Flush(logging::BLOG_INFO);

    // std::cout << "MyLogSink.ThreadFlushLog end ";
    return NULL;
}

}  // namespace nbd
}  // namespace cyprestore
