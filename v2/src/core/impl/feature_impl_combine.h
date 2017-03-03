//
// Created by Arseny Tolmachev on 2017/03/03.
//

#ifndef JUMANPP_FEATURE_IMPL_COMBINE_H
#define JUMANPP_FEATURE_IMPL_COMBINE_H

#include <array>
#include "core/impl/feature_impl_types.h"
#include "core/impl/feature_types.h"
#include "util/hashing.h"

namespace jumanpp {
namespace core {
namespace features {
namespace impl {

class DynamicPatternFeatureImpl {
  i32 index;
  util::ArraySlice<i32> arguments;

 public:
  template <size_t sz>
  constexpr DynamicPatternFeatureImpl(i32 ind, const i32 (&arr)[sz])
      : index{ind}, arguments{arr} {}
  void apply(util::ArraySlice<u64> features,
             util::MutableArraySlice<u64> *result) const noexcept {
    result->at(index) = util::hashing::hashIndexedSeq(
        PatternFeatureSeed ^ index, features, arguments);
  }
};

template <int N>
class NgramFeatureImpl {
  i32 index;
  std::array<i32, N> storage;

 public:
  template <typename... Args>
  constexpr NgramFeatureImpl(i32 idx, Args... arr)
      : index{idx}, storage{{arr...}} {
    static_assert(sizeof...(arr) == N,
                  "number of arguments of ngram feature must be equal to order "
                  "+ 1 (index of feature)");
  }

  void apply(util::MutableArraySlice<u64> *result,
             const util::ArraySlice<u64> &t2, const util::ArraySlice<u64> &t1,
             const util::ArraySlice<u64> &t0) const noexcept;
};

template <>
inline void NgramFeatureImpl<1>::apply(util::MutableArraySlice<u64> *result,
                                       const util::ArraySlice<u64> &t2,
                                       const util::ArraySlice<u64> &t1,
                                       const util::ArraySlice<u64> &t0) const
    noexcept {
  auto p0 = storage[0];
  auto v0 = t0.at(p0);
  result->at(index) = util::hashing::hashCtSeq(UnigramSeed, v0);
};

template <>
inline void NgramFeatureImpl<2>::apply(util::MutableArraySlice<u64> *result,
                                       const util::ArraySlice<u64> &t2,
                                       const util::ArraySlice<u64> &t1,
                                       const util::ArraySlice<u64> &t0) const
    noexcept {
  auto p0 = storage[0];
  auto v0 = t0.at(p0);
  auto p1 = storage[1];
  auto v1 = t1.at(p1);
  result->at(index) =
      util::hashing::hashCtSeq(BigramSeed, v0, v1);
};

template <>
inline void NgramFeatureImpl<3>::apply(util::MutableArraySlice<u64> *result,
                                       const util::ArraySlice<u64> &t2,
                                       const util::ArraySlice<u64> &t1,
                                       const util::ArraySlice<u64> &t0) const
    noexcept {
  auto p0 = storage[0];
  auto v0 = t0.at(p0);
  auto p1 = storage[1];
  auto v1 = t1.at(p1);
  auto p2 = storage[2];
  auto v2 = t2.at(p2);
  result->at(index) = util::hashing::hashCtSeq(TrigramSeed, v0, v1, v2);
};

}  // impl
}  // features
}  // core
}  // jumanpp

#endif  // JUMANPP_FEATURE_IMPL_COMBINE_H
