#pragma once

#include <iosfwd>
#include <string>

#include "Types.h"
#include "copyable.h"

namespace dws {

class StringArg : public copyable {
 private:
    const char *str_;

 public:
    StringArg(const char *str)  // NOLINT
        : str_(str) {}
    StringArg(const std::string &str)  // NOLINT
        : str_(str.c_str()) {}
    const char *c_str() const { return str_; }
};

class StringPiece : public copyable {
 private:
    const char *ptr_;
    int length_;

 public:
    StringPiece() : ptr_(nullptr), length_(0) {}
    explicit StringPiece(const char *str) : ptr_(str), length_(static_cast<int>(strlen(ptr_))) {}
    explicit StringPiece(const unsigned char *str)
        : ptr_(reinterpret_cast<const char *>(str)), length_(static_cast<int>(strlen(ptr_))) {}
    explicit StringPiece(const std::string &str)
        : ptr_(str.data()), length_(static_cast<int>(str.size())) {}
    explicit StringPiece(const char *offset, int len) : ptr_(offset), length_(len) {}

    // unsafe, 因为可能会返回一个非 '\0' 结尾的 C 风格字符串
    const char *data() const { return ptr_; }
    int size() const { return length_; }
    bool empty() const { return length_ == 0; }
    const char *begin() const { return ptr_; }
    const char *end() const { return ptr_ + length_; }

    void clear() {
        ptr_ = nullptr;
        length_ = 0;
    }

    void set(const char *buffer, int len) {
        ptr_ = buffer;
        length_ = len;
    }

    void set(const char *str) {
        ptr_ = str;
        length_ = static_cast<int>(strlen(str));
    }

    void set(const void *buffer, int len) {
        ptr_ = reinterpret_cast<const char *>(buffer);
        length_ = len;
    }

    char operator[](int i) const { return ptr_[i]; }

    void remove_prefix(int n) {
        ptr_ += n;
        length_ -= n;
    }

    void remove_suffix(int n) { length_ -= n; }

    bool operator==(const StringPiece &x) const {
        return ((length_ == x.length_) && (memcmp(ptr_, x.ptr_, length_) == 0));
    }

    bool operator!=(const StringPiece &x) const { return !(*this == x); }

#define STRINGPIECE_BINARY_PREDICATE(cmp, auxcmp)                                \
    bool operator cmp(const StringPiece &x) const {                              \
        int r = memcmp(ptr_, x.ptr_, length_ < x.length_ ? length_ : x.length_); \
        return ((r == 0) ? (length_ auxcmp x.length_) : (r cmp 0));              \
    }
    STRINGPIECE_BINARY_PREDICATE(<, <);
    STRINGPIECE_BINARY_PREDICATE(<=, <);
    STRINGPIECE_BINARY_PREDICATE(>=, >);
    STRINGPIECE_BINARY_PREDICATE(>, >);
#undef STRINGPIECE_BINARY_PREDICATE

    int compare(const StringPiece &x) const {
        int r = memcmp(ptr_, x.ptr_, length_ < x.length_ ? length_ : x.length_);
        if (r == 0) {
            if (length_ < x.length_)
                r = -1;
            else if (length_ > x.length_)
                r = +1;
        }
        return r;
    }

    std::string as_string() const { return std::string(data(), size()); }

    void CopyToString(std::string *target) const { target->assign(ptr_, length_); }

    bool starts_with(const StringPiece &x) const {
        return ((length_ >= x.length_) && (memcmp(ptr_, x.ptr_, x.length_) == 0));
    }
};

}  // namespace dws

// ------------------------------------------------------------------
// Functions used to create STL containers that use StringPiece
//  Remember that a StringPiece's lifetime had better be less than
//  that of the underlying string or char*.  If it is not, then you
//  cannot safely store a StringPiece into an STL container
// ------------------------------------------------------------------

#ifdef HAVE_TYPE_TRAITS
// This makes vector<StringPiece> really fast for some STL implementations
template <>
struct __type_traits<dws::StringPiece> {
    typedef __true_type has_trivial_default_constructor;
    typedef __true_type has_trivial_copy_constructor;
    typedef __true_type has_trivial_assignment_operator;
    typedef __true_type has_trivial_destructor;
    typedef __true_type is_POD_type;
};
#endif

// allow StringPiece to be logged
std::ostream &operator<<(std::ostream &o, const dws::StringPiece &piece);
