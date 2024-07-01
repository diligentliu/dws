#include "Thread.h"

#include <linux/unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <memory>
#include <utility>

#include "CurrentThread.h"
#include "Exception.h"
#include "Timestamp.h"

namespace dws {
namespace detail {

pid_t gettid() { return static_cast<pid_t>(::syscall(SYS_gettid)); }

void afterFork() {
    dws::CurrentThread::t_cachedTid = 0;
    dws::CurrentThread::t_threadName = "main";
    CurrentThread::tid();
}

class ThreadNameInitializer {
 public:
    ThreadNameInitializer() {
        dws::CurrentThread::t_threadName = "main";
        CurrentThread::tid();
        pthread_atfork(nullptr, nullptr, &afterFork);
    }
};

ThreadNameInitializer initializer;

struct ThreadData {
    using ThreadFunc = dws::Thread::ThreadFunc;
    ThreadFunc func_;
    std::string name_;
    pid_t* tid_;
    sem_t* sem_;

    ThreadData(ThreadFunc func, std::string name, pid_t* tid, sem_t* sem)
        : func_(std::move(func)), name_(std::move(name)), tid_(tid), sem_(sem) {}

    void runInThread() {
        *tid_ = CurrentThread::tid();
        tid_ = nullptr;
        sem_post(sem_);
        sem_ = nullptr;

        CurrentThread::t_threadName = name_.empty() ? "dwsThread" : name_.c_str();
        ::prctl(PR_SET_NAME, CurrentThread::t_threadName);
        try {
            func_();
            CurrentThread::t_threadName = "finished";
        } catch (const Exception& ex) {
            CurrentThread::t_threadName = "crashed";
            fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
            fprintf(stderr, "reason: %s\n", ex.what());
            fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
        } catch (const std::exception& ex) {
            CurrentThread::t_threadName = "crashed";
            fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
            fprintf(stderr, "reason: %s\n", ex.what());
            abort();
        } catch (...) {
            CurrentThread::t_threadName = "crashed";
            fprintf(stderr, "unknown exception caught in Thread %s\n", name_.c_str());
            throw;  // rethrow
        }
    }
};

void* startThread(void* obj) {
    ThreadData* data = reinterpret_cast<ThreadData*>(obj);
    data->runInThread();
    delete data;
    return nullptr;
}

}  // namespace detail

void CurrentThread::cacheTid() {
    if (t_cachedTid == 0) {
        t_cachedTid = detail::gettid();
        t_tidStringLength = snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid);
    }
}

bool CurrentThread::isMainThread() { return tid() == ::gettid(); }

void CurrentThread::sleepUsec(int64_t usec) {
    struct timespec ts {};
    ts.tv_sec = static_cast<time_t>(usec / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<int32_t>(usec % Timestamp::kMicroSecondsPerSecond);
    ::nanosleep(&ts, nullptr);
}

std::atomic_int Thread::numCreated_(0);

Thread::Thread(ThreadFunc func, std::string name)
    : started_(false),
      joined_(false),
      thread_(nullptr),
      tid_(0),
      func_(std::move(func)),
      name_(std::move(name)) {
    sem_init(&sem, 0, 0);
    setDefaultName();
}

Thread::~Thread() {
    if (started_ && !joined_) {
        thread_->detach();
    }
    sem_destroy(&sem);
}

void Thread::start() {
    assert(!started_);
    started_ = true;
    auto data = std::make_unique<detail::ThreadData>(func_, name_, &tid_, &sem);
    thread_ = std::make_shared<std::thread>(&detail::startThread, data.get());
    sem_wait(&sem);
    assert(tid_ > 0);
}

void Thread::join() {
    assert(started_);
    assert(!joined_);
    joined_ = true;
    thread_->join();
}

void Thread::setDefaultName() {
    int num = ++numCreated_;
    if (name_.empty()) {
        char buf[32];
        bzero(buf, sizeof buf);
        snprintf(buf, sizeof buf, "Thread%d", num);
        name_ = buf;
    }
}

}  // namespace dws
