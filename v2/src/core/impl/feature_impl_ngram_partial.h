//
// Created by Arseny Tolmachev on 2017/10/13.
//

#ifndef JUMANPP_FEATURE_IMPL_NGRAM_PARTIAL_H
#define JUMANPP_FEATURE_IMPL_NGRAM_PARTIAL_H

#include "core/features_api.h"
#include "core/impl/feature_impl_types.h"
#include "util/printer.h"
#include "util/seahash.h"

namespace jumanpp {
namespace core {
namespace features {
namespace impl {

namespace h = util::hashing;

class UnigramFeature {
  u32 target_;
  u32 index_;
  u32 t0idx_;

 public:
  constexpr UnigramFeature(u32 target, u32 index, u32 t0idx) noexcept
      : target_{target}, index_{index}, t0idx_{t0idx} {}

  JPP_ALWAYS_INLINE void step0(util::ArraySlice<u64> patterns,
                               util::MutableArraySlice<u32> result) const
      noexcept {
    auto t0 = patterns.at(t0idx_);
    auto v = h::seaHashSeq(UnigramSeed, index_, t0);
    result.at(target_) = static_cast<u32>(v);
  }

  void writeMember(util::io::Printer& p, i32 count) const;
};

class BigramFeature {
  u32 target_;
  u32 index_;
  u32 t0idx_;
  u32 t1idx_;

  static constexpr u64 TotalHashArgs = 4;

 public:
  constexpr BigramFeature(u32 target, u32 index, u32 t0idx, u32 t1idx) noexcept
      : target_{target}, index_{index}, t0idx_{t0idx}, t1idx_{t1idx} {}

  JPP_ALWAYS_INLINE void step0(util::ArraySlice<u64> patterns,
                               util::MutableArraySlice<u64> state) const
      noexcept {
    auto t0 = patterns.at(t0idx_);
    auto v = h::rawSeahashStart(TotalHashArgs, BigramSeed, index_, t0);
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

  void writeMember(util::io::Printer& p, i32 count) const;
};

class TrigramFeature {
  u32 target_;
  u32 index_;
  u32 t0idx_;
  u32 t1idx_;
  u32 t2idx_;

  static constexpr u64 TotalHashArgs = 5;

 public:
  constexpr TrigramFeature(u32 target, u32 index, u32 t0idx, u32 t1idx,
                           u32 t2idx) noexcept
      : target_{target},
        index_{index},
        t0idx_{t0idx},
        t1idx_{t1idx},
        t2idx_{t2idx} {}

  JPP_ALWAYS_INLINE void step0(util::ArraySlice<u64> patterns,
                               util::MutableArraySlice<u64> state) const
      noexcept {
    auto t0 = patterns.at(t0idx_);
    auto v = h::rawSeahashStart(TotalHashArgs, TrigramSeed, index_, t0);
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

  void writeMember(util::io::Printer& p, i32 count) const;
};

template <typename Child>
class PartialNgramFeatureApplyImpl : public PartialNgramFeatureApply {
  const Child& child() const { return static_cast<const Child&>(*this); }

 public:
  void applyUni(util::ConstSliceable<u64> p0,
                util::Sliceable<u32> results) const noexcept override {
    for (auto row = 0; row < p0.numRows(); ++row) {
      auto patterns = p0.row(row);
      auto result = results.row(row);
      child().uniStep0(patterns, result);
    }
  }

  void applyBiStep1(util::ConstSliceable<u64> p0,
                    util::Sliceable<u64> result) const noexcept override {
    for (auto row = 0; row < p0.numRows(); ++row) {
      auto patterns = p0.row(row);
      auto state = result.row(row);
      child().biStep0(patterns, state);
    }
  }

  void applyBiStep2(util::ConstSliceable<u64> states, util::ArraySlice<u64> p1,
                    util::Sliceable<u32> results) const noexcept override {
    for (auto row = 0; row < states.numRows(); ++row) {
      auto state = states.row(row);
      auto result = results.row(row);
      child().biStep1(p1, state, result);
    }
  }

  void applyTriStep1(util::ConstSliceable<u64> p0,
                     util::Sliceable<u64> result) const noexcept override {
    for (auto row = 0; row < p0.numRows(); ++row) {
      auto patterns = p0.row(row);
      auto state = result.row(row);
      child().triStep0(patterns, state);
    }
  }

  void applyTriStep2(util::ConstSliceable<u64> states, util::ArraySlice<u64> p1,
                     util::Sliceable<u64> results) const noexcept override {
    for (auto row = 0; row < states.numRows(); ++row) {
      auto state = states.row(row);
      auto result = results.row(row);
      child().triStep1(p1, state, result);
    }
  }

  void applyTriStep3(util::ConstSliceable<u64> states, util::ArraySlice<u64> p2,
                     util::Sliceable<u32> results) const noexcept override {
    for (auto row = 0; row < states.numRows(); ++row) {
      auto state = states.row(row);
      auto result = results.row(row);
      child().triStep2(p2, state, result);
    }
  }
};

class PartialNgramDynamicFeatureApply
    : public PartialNgramFeatureApplyImpl<PartialNgramDynamicFeatureApply> {
  std::vector<UnigramFeature> unigrams_;
  std::vector<BigramFeature> bigrams_;
  std::vector<TrigramFeature> trigrams_;

 public:
  bool outputClassBody(util::io::Printer& p) const;

  Status addChild(const NgramFeature& nf);

  void uniStep0(util::ArraySlice<u64> patterns,
                util::MutableArraySlice<u32> result) const noexcept;

  void biStep0(util::ArraySlice<u64> patterns,
               util::MutableArraySlice<u64> state) const noexcept;

  void biStep1(util::ArraySlice<u64> patterns, util::ArraySlice<u64> state,
               util::MutableArraySlice<u32> result) const noexcept;

  void triStep0(util::ArraySlice<u64> patterns,
                util::MutableArraySlice<u64> state) const noexcept;

  void triStep1(util::ArraySlice<u64> patterns, util::ArraySlice<u64> state,
                util::MutableArraySlice<u64> result) const noexcept;

  void triStep2(util::ArraySlice<u64> patterns, util::ArraySlice<u64> state,
                util::MutableArraySlice<u32> result) const noexcept;
};

}  // namespace impl
}  // namespace features
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_FEATURE_IMPL_NGRAM_PARTIAL_H
