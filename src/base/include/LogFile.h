#pragma once

#include <string>
#include <mutex>
#include <memory>

#include "noncopyable.h"
#include "Types.h"

namespace dws {

namespace FileUtil {
class AppendFile;
}  // namespace FileUtil

class LogFile : noncopyable {
 private:
    static const int kRollPerSeconds_ = 60 * 60 * 24;

    void append_unlocked(const char *logline, int len);

    static std::string getLogFileName(const std::string &basename, time_t *now);

    const std::string basename_;
    const off_t rollSize_;
    const int flushInterval_;
    const int checkEveryN_;

    int count_;

    std::unique_ptr<std::mutex> mutex_;
    time_t startOfPeriod_;
    time_t lastRoll_;
    time_t lastFlush_;
    std::unique_ptr<FileUtil::AppendFile> file_;

 public:
    LogFile(const std::string &basename, off_t rollSize,
            bool threadSafe = true, int flushInterval = 3,
            int checkEveryN = 1024);
    ~LogFile();

    void append(const char *logline, int len);
    void flush();
    bool rollFile();
};

}  // namespace dws
