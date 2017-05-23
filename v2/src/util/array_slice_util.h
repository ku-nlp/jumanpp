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
  if (toErase.empty()) {
    return;
  }
  using std::swap;
  auto erasingIdx = 1;
  auto copyTarget = toErase[0];
  JPP_DCHECK_GE(copyTarget, 0);
  auto copySrc = copyTarget + 1;
  for (; copySrc < data.size() && erasingIdx < toErase.size(); ++copySrc) {
    JPP_DCHECK_LT(copyTarget, copySrc);
    if (copySrc == toErase[erasingIdx]) {
      erasingIdx += 1;
      continue;
    }

    swap(data[copyTarget], data[copySrc]);
    copyTarget += 1;
  }
  while (copySrc < data.size()) {
    JPP_DCHECK_LT(copyTarget, copySrc);
    swap(data[copyTarget], data[copySrc]);
    ++copySrc;
    ++copyTarget;
  }
};

}  // namespace util
}  // namespace jumanpp

#endif  // JUMANPP_ARRAY_SLICE_UTIL_H
