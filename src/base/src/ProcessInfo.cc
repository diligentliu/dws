#include "ProcessInfo.h"

#include <dirent.h>
#include <pwd.h>
#include <sys/resource.h>
#include <sys/times.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>

#include "CurrentThread.h"
#include "FileUtil.h"

namespace dws {
namespace detail {

__thread int t_numOpenedFiles = 0;

int fdDirFilter(const struct dirent *d) {
    if (::isdigit(d->d_name[0])) {
        ++t_numOpenedFiles;
    }
    return 0;
}

__thread std::vector<pid_t> *t_pids = nullptr;

int taskDirFilter(const struct dirent *d) {
    if (::isdigit(d->d_name[0])) {
        t_pids->push_back(atoi(d->d_name));
    }
    return 0;
}

int scanDir(const char *dirpath, int (*filter)(const struct dirent *)) {
    struct dirent **namelist = nullptr;
    int result = ::scandir(dirpath, &namelist, filter, alphasort);
    assert(namelist == nullptr);
    return result;
}

Timestamp g_startTime = Timestamp::now();
int g_clockTicks = static_cast<int>(sysconf(_SC_CLK_TCK));
int g_pageSize = static_cast<int>(sysconf(_SC_PAGE_SIZE));

}  // namespace detail

pid_t ProcessInfo::pid() { return ::getpid(); }

std::string ProcessInfo::pidString() {
    char buf[32];
    snprintf(buf, sizeof buf, "%d", pid());
    return buf;
}

uid_t ProcessInfo::uid() { return ::getuid(); }

std::string ProcessInfo::username() {
    struct passwd pwd;
    struct passwd *result = NULL;
    char buf[8192];
    const char *name = "unknownuser";

    getpwuid_r(uid(), &pwd, buf, sizeof buf, &result);
    if (result) {
        name = pwd.pw_name;
    }
    return name;
}

uid_t ProcessInfo::euid() { return ::geteuid(); }
Timestamp ProcessInfo::startTime() { return detail::g_startTime; }
int ProcessInfo::clockTicksPerSecond() { return detail::g_clockTicks; }
int ProcessInfo::pageSize() { return detail::g_pageSize; }

bool ProcessInfo::isDebugBuild() {
#ifdef NDEBUG
    return false;
#else
    return true;
#endif
}

std::string ProcessInfo::hostname() {
    // HOST_NAME_MAX 64
    // _POSIX_HOST_NAME_MAX 255
    char buf[256];
    if (::gethostname(buf, sizeof buf) == 0) {
        buf[sizeof(buf) - 1] = '\0';
        return buf;
    } else {
        return "unknownhost";
    }
}

std::string ProcessInfo::procStatus() {
    std::string result;
    FileUtil::readFile("/proc/self/status", 65536, &result);
    return result;
}

std::string ProcessInfo::procStat() {
    std::string result;
    FileUtil::readFile("/proc/self/stat", 65536, &result);
    return result;
}

std::string ProcessInfo::threadStat() {
    char buf[64];
    snprintf(buf, sizeof buf, "/proc/self/task/%d/stat", CurrentThread::tid());
    std::string result;
    FileUtil::readFile(buf, 65536, &result);
    return result;
}

std::string ProcessInfo::exePath() {
    std::string result;
    char buf[1024];
    ssize_t n = ::readlink("/proc/self/exe", buf, sizeof buf);
    if (n > 0) {
        result.assign(buf, n);
    }
    return result;
}

int ProcessInfo::openedFiles() {
    detail::t_numOpenedFiles = 0;
    detail::scanDir("/proc/self/fd", detail::fdDirFilter);
    return detail::t_numOpenedFiles;
}

int ProcessInfo::maxOpenFiles() {
    struct rlimit rl;
    if (::getrlimit(RLIMIT_NOFILE, &rl)) {
        return openedFiles();
    } else {
        return static_cast<int>(rl.rlim_cur);
    }
}

ProcessInfo::CpuTime ProcessInfo::cpuTime() {
    ProcessInfo::CpuTime t;
    struct tms tms;
    if (::times(&tms) >= 0) {
        const double hz = static_cast<double>(clockTicksPerSecond());
        t.userSeconds = static_cast<double>(tms.tms_utime) / hz;
        t.systemSeconds = static_cast<double>(tms.tms_stime) / hz;
    }
    return t;
}

int ProcessInfo::numThreads() {
    int result = 0;
    std::string status = procStatus();
    size_t pos = status.find("Threads:");
    if (pos != std::string::npos) {
        result = ::atoi(status.c_str() + pos + 8);
    }
    return result;
}

std::vector<pid_t> ProcessInfo::threads() {
    std::vector<pid_t> result;
    detail::t_pids = &result;
    detail::scanDir("/proc/self/task", detail::taskDirFilter);
    detail::t_pids = NULL;
    std::sort(result.begin(), result.end());
    return result;
}

}  // namespace dws
