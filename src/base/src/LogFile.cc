#include "LogFile.h"

#include <cassert>
#include <cstdio>
#include <ctime>

#include "FileUtil.h"
#include "ProcessInfo.h"

namespace dws {

LogFile::LogFile(const std::string &basename, off_t rollSize, bool threadSafe, int flushInterval,
                 int checkEveryN)
    : basename_(basename),
      rollSize_(rollSize),
      flushInterval_(flushInterval),
      checkEveryN_(checkEveryN),
      count_(0),
      mutex_(threadSafe ? new std::mutex : nullptr),
      startOfPeriod_(0),
      lastRoll_(0),
      lastFlush_(0) {
    assert(basename.find('/') == std::string::npos);
    rollFile();
}

LogFile::~LogFile() = default;

void LogFile::append(const char *logline, int len) {
    if (mutex_) {
        std::lock_guard<std::mutex> lock(*mutex_);
        append_unlocked(logline, len);
    } else {
        append_unlocked(logline, len);
    }
}

void LogFile::flush() {
    if (mutex_) {
        std::lock_guard<std::mutex> lock(*mutex_);
        file_->flush();
    } else {
        file_->flush();
    }
}

bool LogFile::rollFile() {
    time_t now = 0;
    std::string filename = getLogFileName(basename_, &now);
    time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;

    if (now > lastRoll_) {
        lastRoll_ = now;
        lastFlush_ = now;
        startOfPeriod_ = start;
        file_.reset(new FileUtil::AppendFile(filename));
        return true;
    }
    return false;
}

void LogFile::append_unlocked(const char *logline, int len) {
    file_->append(logline, len);

    if (file_->writtenBytes() > rollSize_) {
        rollFile();
    } else {
        ++count_;
        if (count_ >= checkEveryN_) {
            count_ = 0;
            time_t now = ::time(nullptr);
            time_t thisPeriod = now / kRollPerSeconds_ * kRollPerSeconds_;
            if (thisPeriod != startOfPeriod_) {
                rollFile();
            } else if (now - lastFlush_ > flushInterval_) {
                lastFlush_ = now;
                file_->flush();
            }
        }
    }
}

std::string LogFile::getLogFileName(const std::string &basename, time_t *now) {
    std::string filename;
    filename.reserve(basename.size() + 64);
    filename = basename;

    char timebuf[32];
    struct tm tm;
    *now = ::time(nullptr);
    localtime_r(now, &tm);
    strftime(timebuf, sizeof(timebuf), ".%Y%m%d-%H%M%S.", &tm);
    filename += timebuf;

    filename += ProcessInfo::hostname();

    char pidbuf[32];
    snprintf(pidbuf, sizeof(pidbuf), ".%d", ProcessInfo::pid());
    filename += pidbuf;

    filename += ".log";

    return filename;
}

}  // namespace dws
