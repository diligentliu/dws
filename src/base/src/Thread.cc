#include "Thread.h"

#include <semaphore.h>

#include "CurrentThread.h"

namespace dws {

std::atomic_int Thread::numCreated_(0);

Thread::Thread(ThreadFunc func, const std::string &name)
    : started_(false),
      joined_(false),
      thread_(nullptr),
      tid_(0),
      func_(std::move(func)),
      name_(name),
      latch_(1) {
    setDefaultName();
}

Thread::~Thread() {
    if (started_ && !joined_) {
        thread_->detach();
    }
}

void Thread::start() {
    started_ = true;
    sem_t sem;
    sem_init(&sem, false, 0);
    thread_ = std::shared_ptr<std::thread>(new std::thread([&]() {
        tid_ = CurrentThread::tid();
        latch_.countDown();
        sem_post(&sem);
        func_();
    }));
    sem_wait(&sem);
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
        memZero(buf, sizeof(buf));
        snprintf(buf, sizeof(buf), "Thread%d", num);
        name_ = buf;
    }
}

}  // namespace dws
