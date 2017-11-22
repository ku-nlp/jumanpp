//
// Created by Arseny Tolmachev on 2017/05/10.
//

#ifndef JUMANPP_LAZY_H
#define JUMANPP_LAZY_H

#include <utility>
#include "common.hpp"
#include "types.hpp"

namespace jumanpp {
namespace util {

template <typename T>
class Lazy {
  static constexpr u64 LazyGuardValue = 0xdeadbeefdeadfeedULL;

  union {
    u64 flag_;
    T value_;
  };

 public:
  Lazy() noexcept : flag_{LazyGuardValue} {}

  Lazy(const Lazy& o) noexcept(std::is_nothrow_copy_constructible<T>::value) {
    if (o.isInitialized()) {
      initialize(o.value_);
    }
  }

  Lazy& operator=(const Lazy& o) noexcept(
      std::is_nothrow_copy_assignable<T>::value) {
    if (o.isInitialized()) {
      value_ = o.value_;
    }
    return *this;
  }

  Lazy(Lazy&& o) noexcept(std::is_nothrow_move_constructible<T>::value) {
    if (o.isInitialized()) {
      initialize(std::move(o.value_));
      o.flag_ = LazyGuardValue;
    }
  }

  Lazy& operator=(Lazy&& o) noexcept(
      std::is_nothrow_move_assignable<T>::value) {
    if (o.isInitialized()) {
      value_ = std::move(o.value_);
      o.flag_ = LazyGuardValue;
    }
    return *this;
  }

  bool isInitialized() const { return flag_ != LazyGuardValue; }

  template <typename... Args>
  void initialize(Args&&... args) {
    if (isInitialized()) {
      value_.~T();
    }
    new (&value_) T{std::forward<Args>(args)...};
  }

  T& value() {
    JPP_DCHECK(isInitialized());
    return value_;
  }

  const T& value() const {
    JPP_DCHECK(isInitialized());
    return value_;
  }

  void destroy() {
    if (isInitialized()) {
      value_.~T();
      flag_ = LazyGuardValue;
    }
  }

  ~Lazy() {
    if (isInitialized()) {
      value_.~T();
    }
  }
};

}  // namespace util
}  // namespace jumanpp

#endif  // JUMANPP_LAZY_H
