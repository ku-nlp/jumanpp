//
// Created by Arseny Tolmachev on 2017/05/23.
//

#ifndef JUMANPP_DEBUG_OUTPUT_H
#define JUMANPP_DEBUG_OUTPUT_H

#include <iostream>
#include "util/array_slice.h"
#include "util/sliceable_array.h"

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
  util::ArraySlice<T> aslice{slice};
  return util::VOutImpl<T>{aslice};
}

}  // namespace jumanpp

#endif  // JUMANPP_DEBUG_OUTPUT_H
