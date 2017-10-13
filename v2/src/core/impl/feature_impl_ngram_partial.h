//
// Created by Arseny Tolmachev on 2017/10/13.
//

#ifndef JUMANPP_FEATURE_IMPL_NGRAM_PARTIAL_H
#define JUMANPP_FEATURE_IMPL_NGRAM_PARTIAL_H

#include "core/features_api.h"
#include "core/impl/feature_impl_types.h"
#include "util/seahash.h"

namespace jumanpp {
namespace core {
namespace features {
namespace impl {

namespace h = util::hashing;

class PartialDymanicFeature {
 public:
  virtual ~PartialDymanicFeature() noexcept = default;
};

class UnigramFeature {
  u32 target_;
  u32 index_;

 public:
  constexpr UnigramFeature(u32 target, u32 index) noexcept
      : target_{target}, index_{index} {}

  JPP_ALWAYS_INLINE void step0(util::ArraySlice<u64> patterns,
                               util::MutableArraySlice<u32> result) const
      noexcept {
    auto t0 = patterns.at(index_);
    auto v = h::seaHashSeq(UnigramSeed, target_, t0);
    result.at(target_) = static_cast<u32>(v);
  }
};

class BigramFeature {
  u32 target_;
  u32 t0idx_;
  u32 t1idx_;

  static constexpr u64 TotalHashArgs = 4;

 public:
  constexpr BigramFeature(u32 target, u32 t0idx, u32 t1idx) noexcept
      : target_{target}, t0idx_{t0idx}, t1idx_{t1idx} {}

  JPP_ALWAYS_INLINE void step0(util::ArraySlice<u64> patterns,
                               util::MutableArraySlice<u64> state) const
      noexcept {
    auto t0 = patterns.at(t0idx_);
    auto v = h::rawSeahashStart(TotalHashArgs, BigramSeed, target_, t0);
    state.at(target_) = v;
  }

  JPP_ALWAYS_INLINE void step1(util::ArraySlice<u64> patterns,
                               util::ArraySlice<u64> state,
                               util::MutableArraySlice<u32> result) const
      noexcept {
    auto t1 = patterns.at(t1idx_);
    auto s = state.at(target_);
    auto v = h::rawSeahashFinish(s, t1);
    result.at(target_) = static_cast<u32>(v);
  }
};

class TrigramFeature {
  u32 t0idx_;
  u32 t1idx_;
  u32 t2idx_;
  u32 target_;

  static constexpr u64 TotalHashArgs = 5;

 public:
  constexpr TrigramFeature(u32 target, u32 t0idx, u32 t1idx, u32 t2idx) noexcept
      : target_{target}, t0idx_{t0idx}, t1idx_{t1idx}, t2idx_{t2idx} {}

  JPP_ALWAYS_INLINE void step0(util::ArraySlice<u64> patterns,
                               util::MutableArraySlice<u64> state) const
      noexcept {
    auto t0 = patterns.at(t0idx_);
    auto v = h::rawSeahashStart(TotalHashArgs, TrigramSeed, target_, t0);
    state.at(target_) = v;
  }

  JPP_ALWAYS_INLINE void step1(util::ArraySlice<u64> patterns,
                               util::ArraySlice<u64> state,
                               util::MutableArraySlice<u64> result) const
      noexcept {
    auto t1 = patterns.at(t1idx_);
    auto s = state.at(target_);
    auto v = h::rawSeahashContinue(s, t1);
    result.at(target_) = v;
  }

  JPP_ALWAYS_INLINE void step2(util::ArraySlice<u64> patterns,
                               util::ArraySlice<u64> state,
                               util::MutableArraySlice<u32> result) const
      noexcept {
    auto t2 = patterns.at(t2idx_);
    auto s = state.at(target_);
    auto v = h::rawSeahashFinish(s, t2);
    result.at(target_) = static_cast<u32>(v);
  }
};

class PartialNgramDynamicFeatureApply {
  std::vector<std::unique_ptr<PartialDymanicFeature>> features_;
};

}  // namespace impl
}  // namespace features
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_FEATURE_IMPL_NGRAM_PARTIAL_H
