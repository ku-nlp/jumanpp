//
// Created by Arseny Tolmachev on 2017/11/15.
//

#ifndef JUMANPP_FEATURE_NGRAM_PARTIAL_KERNELS_H
#define JUMANPP_FEATURE_NGRAM_PARTIAL_KERNELS_H

#include "core/analysis/perceptron.h"
#include "util/common.hpp"
#include "util/fast_hash.h"
#include "util/sliceable_array.h"
#include "util/types.hpp"

namespace jumanpp {
namespace core {
namespace features {
namespace impl {

inline void applyBiTriFullKernel(
    util::ArraySlice<u64> biState, util::ArraySlice<u64> triState,
    util::ConstSliceable<u64> t1pats, util::ConstSliceable<u64> t2pats,
    util::ArraySlice<u32> t1idxes, util::ArraySlice<u32> t1featuresBi,
    util::ArraySlice<u32> t1FeaturesTri, util::ArraySlice<u32> t2FeaturesTri,
    util::MutableArraySlice<u32> buf1, util::MutableArraySlice<u32> buf2,
    const analysis::WeightBuffer& weights,
    util::MutableArraySlice<float> scoreBuffer,
    util::MutableArraySlice<float> result) {
  u32 mask = static_cast<u32>(weights.size() - 1);
  auto numBiFeat = t1featuresBi.size();

  int biRow = 0;
  for (; biRow < t1pats.numRows(); ++biRow) {
    buf1.swap(buf2);
    auto t1row = t1pats.row(biRow);
    float r1 = 0;
    float r2 = 0;
    u32 feat = 0;
    for (; (feat + 2) <= numBiFeat; feat += 2) {
      auto f = feat;
      auto t1v1 = t1row.at(t1featuresBi.at(f));
      auto v1 = util::hashing::FastHash1{biState.at(f)}.mix(t1v1).masked(mask);
      weights.prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(v1);
      buf1.at(f) = v1;
      r1 += weights.at(buf2.at(f));
      f += 1;
      auto t1v2 = t1row.at(t1featuresBi.at(f));
      auto v2 = util::hashing::FastHash1{biState.at(f)}.mix(t1v2).masked(mask);
      weights.prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(v2);
      buf1.at(f) = v2;
      r2 += weights.at(buf2.at(f));
    }
    if (numBiFeat & 0x1) {
      auto f = feat;
      auto t1v1 = t1row.at(t1featuresBi.at(f));
      auto v1 = util::hashing::FastHash1{biState.at(f)}.mix(t1v1).masked(mask);
      weights.prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(v1);
      buf1.at(f) = v1;
      r1 += weights.at(buf2.at(f));
    }
    if (JPP_LIKELY(biRow > 0)) {
      scoreBuffer.at(biRow - 1) = r1 + r2;
    }
  }

  int triRow = 0;
  auto numTriFeat = t2FeaturesTri.size();
  util::MutableArraySlice<u32> tribuf1{buf1.data(), numTriFeat};
  util::MutableArraySlice<u32> tribuf2{buf2.data(), numTriFeat};
  for (; triRow < t2pats.numRows(); ++triRow) {
    tribuf1.swap(tribuf2);
    auto t2row = t2pats.row(triRow);
    auto t1row = t1pats.row(t1idxes.at(triRow));
    float r1 = 0;
    float r2 = 0;
    u32 feat = 0;
    for (; (feat + 2) <= numTriFeat; feat += 2) {
      auto f = feat;
      auto t1v1 = t1row.at(t1FeaturesTri.at(f));
      auto t2v1 = t2row.at(t2FeaturesTri.at(f));
      auto v1 =
          util::hashing::FastHash1{triState.at(f)}.mix(t1v1).mix(t2v1).masked(
              mask);
      weights.prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(v1);
      tribuf1.at(f) = v1;
      r1 += weights.at(tribuf2.at(f));
      f += 1;
      auto t1v2 = t1row.at(t1FeaturesTri.at(f));
      auto t2v2 = t2row.at(t2FeaturesTri.at(f));
      auto v2 =
          util::hashing::FastHash1{triState.at(f)}.mix(t1v2).mix(t2v2).masked(
              mask);
      weights.prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(v2);
      tribuf1.at(f) = v2;
      r2 += weights.at(tribuf2.at(f));
    }
    if (numTriFeat & 0x1) {
      auto f = feat;
      auto t1v1 = t1row.at(t1FeaturesTri.at(f));
      auto t2v1 = t2row.at(t2FeaturesTri.at(f));
      auto v1 =
          util::hashing::FastHash1{triState.at(f)}.mix(t1v1).mix(t2v1).masked(
              mask);
      weights.prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(v1);
      tribuf1.at(f) = v1;
      r1 += weights.at(tribuf2.at(f));
    }
    if (JPP_LIKELY(triRow > 0)) {
      result.at(triRow - 1) = scoreBuffer.at(t1idxes.at(triRow - 1)) + r1 + r2;
    } else {
      scoreBuffer.at(biRow - 1) =
          analysis::impl::computeUnrolled4RawPerceptron(weights, buf1);
    }
  }
  result.at(triRow - 1) =
      scoreBuffer.at(t1idxes.at(triRow - 1)) +
      analysis::impl::computeUnrolled4RawPerceptron(weights, tribuf1);
}

}  // namespace impl
}  // namespace features
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_FEATURE_NGRAM_PARTIAL_KERNELS_H
