//
// Created by Arseny Tolmachev on 2017/11/02.
//

#ifndef JUMANPP_INT_SEQ_UTIL_H
#define JUMANPP_INT_SEQ_UTIL_H

#include "util/fast_hash.h"
#include "util/inlined_vector.h"

namespace jumanpp {
namespace core {
namespace impl {

using FeatureBuffer = jumanpp::util::InlinedVector<i32, 16>;

struct DicEntryFeatures {
  FeatureBuffer features;

  bool operator==(const DicEntryFeatures& o) const {
    if (features.size() != o.features.size()) {
      return false;
    }

    for (int i = 0; i < features.size(); ++i) {
      if (features[i] != o.features[i]) {
        return false;
      }
    }
    return true;
  }
};

struct DicEntryFeaturesHash {
  size_t operator()(const DicEntryFeatures& cf) const {
    auto h = util::hashing::FastHash1{};
    for (auto f : cf.features) {
      h = h.mix(f);
    }
    return h.mix(cf.features.size()).result();
  }
};

}  // namespace impl
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_INT_SEQ_UTIL_H
