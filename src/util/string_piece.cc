//
// Created by Arseny Tolmachev on 2017/02/18.
//

#include "string_piece.h"
#include <cstring>
#include <iostream>

namespace jumanpp {

std::ostream &operator<<(std::ostream &str, const StringPiece &sp) {
  str.write(sp.char_begin(), sp.size());
  return str;
}

bool operator==(const StringPiece &l, const StringPiece &r) {
  if (l.size() != r.size()) return false;
  return std::strncmp(l.char_begin(), r.char_begin(), l.size()) == 0;
}

bool operator<(StringPiece l, StringPiece r) {
  auto min_size = std::min(l.size(), r.size());
  int res = std::strncmp(l.char_begin(), r.char_begin(), min_size);
  if (res == 0) {
    return l.size() < r.size();
  }
  return res < 0;
}

//
// StringPiece::StringPiece<void>(StringPiece::char_ptr ptr) noexcept
//    : StringPiece(ptr, ptr + std::strlen(ptr)) {}

StringPiece StringPiece::fromCString(const char *data) {
  return StringPiece{data, data + std::strlen(data)};
}

}  // namespace jumanpp
