//
// Created by Arseny Tolmachev on 2017/02/28.
//

#ifndef JUMANPP_STL_UTIL_H
#define JUMANPP_STL_UTIL_H

#include <initializer_list>

namespace jumanpp {
namespace util {

template <typename C, typename I>
inline bool contains(const C& c, const I& i) {
  for (auto&& x : c) {
    if (x == i) {
      return true;
    }
  }
  return false;
}

template <typename T>
inline bool contains(std::initializer_list<T> data, const T& obj) {
  return contains<std::initializer_list<T>, T>(data, obj);
}

}  // util
}  // jumanpp

#endif  // JUMANPP_STL_UTIL_H
