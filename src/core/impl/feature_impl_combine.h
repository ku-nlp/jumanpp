//
// Created by Arseny Tolmachev on 2017/03/03.
//

#ifndef JUMANPP_FEATURE_IMPL_COMBINE_H
#define JUMANPP_FEATURE_IMPL_COMBINE_H

#include <array>

#include "core/features_api.h"
#include "core/impl/feature_impl_hasher.h"
#include "core/impl/feature_impl_types.h"
#include "core/impl/feature_types.h"
#include "util/codegen.h"
#include "util/fast_hash.h"
#include "util/hashing.h"

namespace jumanpp {
namespace core {
namespace features {
namespace impl {

template <int N>
class NgramFeatureImpl {
 public:
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

  void apply(util::MutableArraySlice<u32> result,
             const util::ArraySlice<u64> &t2, const util::ArraySlice<u64> &t1,
             const util::ArraySlice<u64> &t0) const noexcept;
};

template <>
inline void NgramFeatureImpl<1>::apply(
    util::MutableArraySlice<u32> result, const util::ArraySlice<u64> &t2,
    const util::ArraySlice<u64> &t1,
    const util::ArraySlice<u64> &t0) const noexcept {
  auto p0 = storage[0];
  auto v0 = t0.at(p0);
  auto ret = Hasher{}.mix(3).mix(index).mix(UnigramSeed).mix(v0);
  result.at(index) = static_cast<u32>(ret.result());
};

template <>
inline void NgramFeatureImpl<2>::apply(
    util::MutableArraySlice<u32> result, const util::ArraySlice<u64> &t2,
    const util::ArraySlice<u64> &t1,
    const util::ArraySlice<u64> &t0) const noexcept {
  auto p0 = storage[0];
  auto v0 = t0.at(p0);
  auto p1 = storage[1];
  auto v1 = t1.at(p1);
  auto ret = Hasher{}.mix(4).mix(index).mix(BigramSeed).mix(v0).mix(v1);
  result.at(index) = static_cast<u32>(ret.result());
};

template <>
inline void NgramFeatureImpl<3>::apply(
    util::MutableArraySlice<u32> result, const util::ArraySlice<u64> &t2,
    const util::ArraySlice<u64> &t1,
    const util::ArraySlice<u64> &t0) const noexcept {
  auto p0 = storage[0];
  auto v0 = t0.at(p0);
  auto p1 = storage[1];
  auto v1 = t1.at(p1);
  auto p2 = storage[2];
  auto v2 = t2.at(p2);
  auto ret =
      Hasher{}.mix(5).mix(index).mix(TrigramSeed).mix(v0).mix(v1).mix(v2);
  result.at(index) = static_cast<u32>(ret.result());
};

class DynamicNgramFeature : public FeatureApply {
 public:
  virtual void apply(util::MutableArraySlice<u32> result,
                     const util::ArraySlice<u64> &t2,
                     const util::ArraySlice<u64> &t1,
                     const util::ArraySlice<u64> &t0) const noexcept = 0;

  virtual Status emitCode(util::codegen::MethodBody *cls) const = 0;
};

template <size_t N>
class NgramFeatureDynamicAdapter : public DynamicNgramFeature {
  NgramFeatureImpl<N> impl;

 public:
  template <typename... Args>
  NgramFeatureDynamicAdapter(i32 index, Args... rest) : impl{index, rest...} {}

  virtual void apply(util::MutableArraySlice<u32> result,
                     const util::ArraySlice<u64> &t2,
                     const util::ArraySlice<u64> &t1,
                     const util::ArraySlice<u64> &t0) const noexcept override {
    impl.apply(result, t2, t1, t0);
  }

  virtual Status emitCode(util::codegen::MethodBody *cls) const override {
    u64 hashSeed = 0;
    switch (N) {
      case 1:
        hashSeed = UnigramSeed;
        break;
      case 2:
        hashSeed = BigramSeed;
        break;
      case 3:
        hashSeed = TrigramSeed;
        break;
      default:
        return Status::InvalidParameter() << "NGram Feature Codegen: " << N
                                          << " is incorrect order of N-gram";
    }

    auto &bldr = cls->resultInto(impl.index)
                     .addHashConstant(impl.index)
                     .addHashConstant(hashSeed);
    if (N >= 1) {
      bldr.addHashIndexed("t0", impl.storage[0]);
    }
    if (N >= 2) {
      bldr.addHashIndexed("t1", impl.storage[1]);
    }
    if (N >= 3) {
      bldr.addHashIndexed("t2", impl.storage[2]);
    }

    return Status::Ok();
  }
};

template <typename Child>
class NgramFeatureApplyImpl : public NgramFeatureApply {
 public:
  void applyBatch(impl::NgramFeatureData *data) const noexcept override {
    const Child &child = static_cast<const Child &>(*this);
    while (data->nextT0()) {
      child.apply(data->finalFeatures(), data->patternT2(), data->patternT1(),
                  data->patternT0());
    }
  }
};

class NgramDynamicFeatureApply
    : public NgramFeatureApplyImpl<NgramDynamicFeatureApply> {
  std::vector<std::unique_ptr<DynamicNgramFeature>> children_;

 public:
  Status addChild(const spec::NgramFeatureDescriptor &nf);

  void apply(util::MutableArraySlice<u32> result,
             const util::ArraySlice<u64> &t2, const util::ArraySlice<u64> &t1,
             const util::ArraySlice<u64> &t0) const noexcept {
    for (auto &c : children_) {
      c->apply(result, t2, t1, t0);
    }
  }

  Status emitCode(util::codegen::MethodBody *cls) const {
    for (auto &c : children_) {
      JPP_RETURN_IF_ERROR(c->emitCode(cls));
    }
    return Status::Ok();
  }
};

}  // namespace impl
}  // namespace features
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_FEATURE_IMPL_COMBINE_H
