#pragma once

#include "Types.h"

#include <exception>
#include <string>

namespace dws {

class Exception : public std::exception {
 private:
    std::string message_;
    std::string stack_;

 public:
    explicit Exception(std::string what);
    ~Exception() noexcept override = default;
    // default copy-ctor and operator= are okay.
    const char* what() const noexcept override {
        return message_.c_str();
    }
    const char* stackTrace() const noexcept {
        return stack_.c_str();
    }
};

}  // namespace dws
