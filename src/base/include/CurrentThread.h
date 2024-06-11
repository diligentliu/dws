#pragma once

#include <string>

#include "Types.h"

namespace dws {

namespace CurrentThread {
// internal, in CurrentThread.cpp
extern __thread int t_cachedTid;
extern __thread char t_tidString[32];
extern __thread int t_tidStringLength;
extern __thread const char *t_threadName;

void cacheTid();

inline int tid() {
    // 一个线程第一次调用 tid() 时，会调用 cacheTid()，然后将结果保存在 t_cachedTid 中
    if (__builtin_expect(t_cachedTid == 0, 0)) {
        cacheTid();
    }
    return t_cachedTid;
}

inline const char *tidString() {
    return t_tidString;
}

inline int tidStringLength() {
    return t_tidStringLength;
}

inline const char *name() {
    return t_threadName;
}

bool isMainThread();

void sleepUsec(int64_t usec);

std::string stackTrace(bool demangle);
}  // namespace CurrentThread

}  // namespace dws
