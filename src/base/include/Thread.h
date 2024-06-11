#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>

#include "CountDownLatch.h"

namespace dws {

class Thread : noncopyable {
    using ThreadFunc = std::function<void()>;

 private:
    void setDefaultName();

    bool started_;
    bool joined_;
    std::shared_ptr<std::thread> thread_;
    pid_t tid_;
    ThreadFunc func_;
    std::string name_;
    static std::atomic_int numCreated_;
    CountDownLatch latch_;

 public:
    explicit Thread(ThreadFunc func, const std::string& name = std::string());
    ~Thread();

    void start();
    void join();

    bool started() const { return started_; }
    pid_t tid() const { return tid_; }
    const std::string& name() const { return name_; }

    static int numCreated() { return numCreated_; }
};

}  // namespace dws
