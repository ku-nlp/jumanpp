//
// Created by Arseny Tolmachev on 2017/05/23.
//

#ifndef JUMANPP_DEBUG_OUTPUT_H
#define JUMANPP_DEBUG_OUTPUT_H

#include <iostream>
#include "util/array_slice.h"
#include "util/sliceable_array.h"
#include "util/string_piece.h"

namespace jumanpp {
namespace util {

template <typename T>
struct VOutImpl {
  util::ArraySlice<T> wrapped;
};

template <typename T>
std::ostream& operator<<(std::ostream& os, const VOutImpl<T>& v) {
  os << '[';
  for (int i = 0; i < v.wrapped.size(); ++i) {
    os << v.wrapped[i];
    if (i != v.wrapped.size() - 1) {
      os << ", ";
    }
  }
  os << ']';
  return os;
}

template <typename T>
struct MOutImpl {
  util::ConstSliceable<T> wrapped;
};

template <typename T>
std::ostream& operator<<(std::ostream& os, const MOutImpl<T>& v) {
  auto& m = v.wrapped;
  os << '[';
  os << " nrows=" << m.numRows() << " ncols=" << m.rowSize();
  for (int row = 0; row < m.numRows(); ++row) {
    auto robj = m.row(row);
    os << "\n";
    for (int col = 0; col < m.rowSize(); ++col) {
      os << robj.at(col) << ", ";
    }
  }
  os << ']';
  return os;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const Sliceable<T>& s) {
  os << "\n[";

  for (size_t i = 0; i < s.numRows(); ++i) {
    auto row = s.row(i);
    for (size_t j = 0; j < s.rowSize(); ++j) {
      os << row.at(j) << " ";
    }
    os << "\n ";
  }

  os << "]";
  return os;
}

}  // namespace util

template <typename C>
util::VOutImpl<typename C::value_type> VOut(const C& slice) {
  using T = typename C::value_type;
  util::ArraySlice<T> aslice(slice);
  return util::VOutImpl<T>{aslice};
}

template <typename C>
util::MOutImpl<typename C::value_type> MOut(const C& slice) {
  using T = typename C::value_type;
  return util::MOutImpl<T>{slice};
}

template <typename It>
util::VOutImpl<typename It::value_type> VOut(It beg, It end) {
  using T = typename It::value_type;
  u32 sz = static_cast<u32>(std::distance(beg, end));
  util::ArraySlice<T> aslice{&*beg, sz};
  return util::VOutImpl<T>{aslice};
}

struct HexString {
  StringPiece data;
  HexString(StringPiece o) : data{o} {}
};

inline std::ostream& operator<<(std::ostream& os, const HexString& hs) {
  const char chars[] = "0123456789abcdef";

  StringPiece sp = hs.data;
  size_t len = sp.size();
  os << '[';
  for (int i = 0; i < len; ++i) {
    u8 item = static_cast<u8>(sp.data()[i]);
    os << chars[(item >> 4) & 0xf];
    os << chars[item & 0xf];
    if (i != len - 1) {
      os << ' ';
    }
  }
  os << ']';
  return os;
}

}  // namespace jumanpp

#endif  // JUMANPP_DEBUG_OUTPUT_H
