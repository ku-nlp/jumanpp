//
// Created by Arseny Tolmachev on 2017/11/21.
//

#ifndef JUMANPP_CONFIG_H
#define JUMANPP_CONFIG_H

#include "serialization.h"

namespace jumanpp {
namespace util {

template <typename T>
class Cfg {
  bool isDefault_;
  T value_;

 public:
  Cfg() noexcept(std::is_nothrow_default_constructible<T>::value)
      : isDefault_{true}, value_{} {}
  Cfg(const T& v) noexcept(std::is_nothrow_copy_constructible<T>::value)
      : isDefault_{true}, value_{v} {}
  Cfg(T&& v) noexcept(std::is_nothrow_move_constructible<T>::value)
      : isDefault_{true}, value_{std::move(v)} {}
  operator const T&() const { return value_; }

  Cfg& operator=(const T& v) noexcept(
      std::is_nothrow_copy_assignable<T>::value) {
    value_ = v;
    isDefault_ = false;
    return *this;
  }

  Cfg& operator=(T&& v) noexcept(std::is_nothrow_move_assignable<T>::value) {
    value_ = std::move(v);
    isDefault_ = false;
    return *this;
  }

  Cfg& mergeWith(const Cfg& c) {
    if (!c.isDefault()) {
      isDefault_ = false;
      value_ = c.value();
    }
    return *this;
  }

  bool isDefault() const { return isDefault_; }
  const T& value() const { return value_; }
  T& value() { return value_; }
  void reset(bool to = true) { isDefault_ = to; }

  bool operator==(const Cfg& c) const { return value_ == c.value_; }
};

template <typename T>
bool areAllDefault(const Cfg<T>& c) {
  return c.isDefault();
}

template <typename T, typename... Args>
bool areAllDefault(const Cfg<T>& c, const Args&... args) {
  return c.isDefault() && areAllDefault(args...);
}

namespace serialization {

template <typename T>
struct SerializeImpl<Cfg<T>> {
  static inline void DoSerializeSave(Cfg<T>& o, Saver* s,
                                     util::CodedBuffer& buf) {
    buf.writeVarint(o.isDefault() ? 0 : 1);
    SerializeImpl<T>::DoSerializeSave(o.value(), s, buf);
  }

  static inline bool DoSerializeLoad(Cfg<T>& o, Loader* s,
                                     util::CodedBufferParser& p) {
    u64 v;
    JPP_RET_CHECK(p.readVarint64(&v));
    JPP_RET_CHECK(SerializeImpl<T>::DoSerializeLoad(o.value(), s, p));
    o.reset(v == 0);
    return true;
  }
};

}  // namespace serialization

}  // namespace util
}  // namespace jumanpp

#endif  // JUMANPP_CONFIG_H
