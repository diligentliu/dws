#pragma once

#include <cstdint>
#include <cstring>
#include <string>

#ifndef NDEBUG

#include <cassert>

#endif

namespace dws {

inline void memZero(void *arr, size_t n) {
    memset(arr, 0, n);
}

// 实现隐式转换的显示化
template<typename To, typename From>
inline To implicit_cast(From const &f) {
    return f;
}

// 实现向下转换的编译时检查和运行时检查
template<typename From, typename To>
inline To down_cast(From *f) {
    // 编译时检查
    if (false) {
        implicit_cast<From *, To>(0);
    }
#if !defined(NDEBUG) && !defined(GOOGLE_PROTOBUF_NO_RTTI)
    assert(f == nullptr || dynamic_cast<To>(f) != nullptr);
#endif
    return static_cast<To>(f);
}

}  // namespace dws
