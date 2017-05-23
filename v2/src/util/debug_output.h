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
  const util::ArraySlice<i32> wrapped;
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
  return util::VOutImpl<typename C::value_type>{slice};
}

}  // namespace jumanpp

#endif  // JUMANPP_DEBUG_OUTPUT_H
