//
// Created by Arseny Tolmachev on 2017/03/22.
//

#ifndef JUMANPP_ARRAY_SLICE_UTIL_H
#define JUMANPP_ARRAY_SLICE_UTIL_H

#include "array_slice.h"

namespace jumanpp {
namespace util {

template <typename C1, typename C2>
void compact(C1& data, const C2& toErase) {
  using std::swap;
  JPP_DCHECK_GT(toErase.size(), 0);
  auto erasingIdx = 1;
  auto copyTarget = toErase[0];
  for (auto copySrc = copyTarget + 1; copySrc < data.size(); ++copySrc) {
    if (copySrc == toErase[erasingIdx]) {
      erasingIdx += 1;
      continue;
    }

    swap(data[copyTarget], data[copySrc]);
    copyTarget += 1;
  }
};

}  // util
}  // jumanpp

#endif  // JUMANPP_ARRAY_SLICE_UTIL_H
