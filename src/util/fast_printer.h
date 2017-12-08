//
// Created by Arseny Tolmachev on 2017/11/12.
//

#ifndef JUMANPP_FAST_PRINTER_H
#define JUMANPP_FAST_PRINTER_H

#include "util/format.h"
#include "util/string_piece.h"

namespace jumanpp {
namespace util {
namespace io {

class FastPrinter {
  fmt::BasicMemoryWriter<char> formatter_;

 public:
  FastPrinter& operator<<(const StringPiece& sp) {
    formatter_.buffer().append(sp.begin(), sp.end());
    return *this;
  }

  template <typename T>
  FastPrinter& operator<<(const T& o) {
    formatter_ << o;
    return *this;
  }

  template <typename... Args>
  FastPrinter& write(fmt::BasicCStringRef<char> fmt, const Args&... args) {
    formatter_.write(fmt, args...);
    return *this;
  }

  StringPiece result() const {
    return StringPiece{formatter_.data(), formatter_.size()};
  }

  void reset() { formatter_.clear(); }

  void reserve(size_t size) { formatter_.buffer().reserve(size); }
};

}  // namespace io
}  // namespace util

template <typename T>
fmt::BasicWriter<T>& operator<<(fmt::BasicWriter<T>& f, const StringPiece& sp) {
  f << fmt::BasicStringRef<char>{sp.begin(), sp.size()};
  return f;
}

template <typename T, size_t N>
fmt::BasicWriter<T>& operator<<(fmt::BasicWriter<T>& f, const char (&sp)[N]) {
  StringPiece tmp{sp};
  f << tmp;
  return f;
}

}  // namespace jumanpp

#endif  // JUMANPP_FAST_PRINTER_H
