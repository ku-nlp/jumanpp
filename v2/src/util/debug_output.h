//
// Created by Arseny Tolmachev on 2017/05/23.
//

#ifndef JUMANPP_DEBUG_OUTPUT_H
#define JUMANPP_DEBUG_OUTPUT_H

#include <iostream>
#include "util/array_slice.h"

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

}  // namespace util

template <typename C>
util::VOutImpl<typename C::value_type> VOut(const C& slice) {
  using T = typename C::value_type;
  util::ArraySlice<T> aslice{slice};
  return util::VOutImpl<T>{aslice};
}

}  // namespace jumanpp

#endif  // JUMANPP_DEBUG_OUTPUT_H
