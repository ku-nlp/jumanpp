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

StringPiece::StringPiece(StringPiece::pointer_t begin)
    : begin_{begin},
      end_{begin + std::strlen(reinterpret_cast<char_ptr>(begin))} {}
}
