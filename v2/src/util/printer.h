//
// Created by Arseny Tolmachev on 2017/03/09.
//

#ifndef JUMANPP_PRINTER_H
#define JUMANPP_PRINTER_H

#include <sstream>
#include "util/string_piece.h"

namespace jumanpp {
namespace util {
namespace io {

namespace impl {
class PrinterBuffer final : public std::stringbuf {
 public:
  void reset();
  StringPiece contents() const;
};

class PrinterStream final : public std::ostream {
 public:
  PrinterStream(PrinterBuffer* ptr) : std::ostream(ptr) {}
};
}  // namespace impl

class Printer {
  impl::PrinterBuffer buf_;
  impl::PrinterStream data_;
  i32 currentIndent = 0;

  void putChar(char c) {
    if (c == '\n') {
      data_.put('\n');
      for (int i = 0; i < currentIndent; ++i) {
        data_.put(' ');
      }
    } else {
      data_.put(c);
    }
  }

 public:
  Printer(const Printer&) = delete;

  Printer() : buf_{}, data_{&buf_} {}

  inline Printer& operator<<(const char c) {
    putChar(c);
    return *this;
  }

  // stringpiece is cheap, so always copy
  inline Printer& operator<<(StringPiece s) {
    for (auto c : s) {
      putChar(c);
    }
    return *this;
  }

  // everything non-stringable come here
  template <typename T>
  inline typename std::enable_if<!std::is_convertible<T, StringPiece>::value,
                                 Printer>::type&
  operator<<(const T& v) {
    data_ << v;
    return *this;
  }

  template <typename T>
  void inline rawOutput(const T& o) {
    data_ << o;
  }

  void reset() { buf_.reset(); }

  StringPiece result() const { return buf_.contents(); }

  void addIndent(i32 number) {
    currentIndent = std::max(0, currentIndent + number);
  }
};

class Indent {
  Printer* printer;
  i32 indent;

 public:
  Indent(Printer& p, i32 num) : printer{&p}, indent{num} {
    printer->addIndent(indent);
  }

  ~Indent() { printer->addIndent(-indent); }
};

#define JPP_TEXT(x) #x

}  // namespace io
}  // namespace util
}  // namespace jumanpp

#endif  // JUMANPP_PRINTER_H
