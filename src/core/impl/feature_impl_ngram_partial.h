//
// Created by Arseny Tolmachev on 2017/10/13.
//

#ifndef JUMANPP_FEATURE_IMPL_NGRAM_PARTIAL_H
#define JUMANPP_FEATURE_IMPL_NGRAM_PARTIAL_H

#include "core/analysis/perceptron.h"
#include "core/features_api.h"
#include "core/impl/feature_impl_types.h"
#include "util/fast_hash.h"

namespace jumanpp {
namespace core {
namespace features {
namespace impl {

namespace h = util::hashing;

class UnigramFeature {
  u32 target_;
  u32 index_;
  u32 t0idx_;

  static constexpr u64 TotalHashArgs = 3;

 public:
  constexpr UnigramFeature(u32 target, u32 index, u32 t0idx) noexcept
      : target_{target}, index_{index}, t0idx_{t0idx} {}

  JPP_ALWAYS_INLINE u32 maskedValueFor(u64 t0, u32 mask) const noexcept {
    auto v =
        h::FastHash1{}.mix(TotalHashArgs).mix(index_).mix(UnigramSeed).mix(t0);
    return v.masked(mask);
  }

  JPP_ALWAYS_INLINE u32 step0(util::ArraySlice<u64> patterns,
                              util::MutableArraySlice<u32> result,
                              u32 mask) const noexcept {
    auto t0 = patterns.at(t0idx_);
    auto v = maskedValueFor(t0, mask);
    result.at(target_) = v;
    return v;
  }

  constexpr u32 target() const noexcept { return target_; }
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
    auto v =
        h::FastHash1{}.mix(TotalHashArgs).mix(index_).mix(BigramSeed).mix(t0);
    state.at(target_) = v.result();
  }

  JPP_ALWAYS_INLINE u32 step1(util::ArraySlice<u64> patterns,
                              util::ArraySlice<u64> state,
                              util::MutableArraySlice<u32> result,
                              u32 mask) const noexcept {
    auto t1 = patterns.at(t1idx_);
    auto s = state.at(target_);
    auto v = h::FastHash1{s}.mix(t1);
    u32 r = v.masked(mask);
    result.at(target_) = r;
    return r;
  }

  JPP_ALWAYS_INLINE u32 jointApply(util::ArraySlice<u64> t0,
                                   util::ArraySlice<u64> t1,
                                   util::MutableArraySlice<u32> result,
                                   u32 mask) const noexcept {
    u32 r = jointRaw(t0, t1, mask);
    result.at(target_) = r;
    return r;
  }

  JPP_ALWAYS_INLINE u32 raw2(u64 state, util::ArraySlice<u64> t1,
                             u32 mask) const noexcept {
    auto t1v = t1.at(t1idx_);
    auto v = h::FastHash1{state}.mix(t1v);
    return v.masked(mask);
  }

  JPP_ALWAYS_INLINE u32 jointRaw(util::ArraySlice<u64> t0,
                                 util::ArraySlice<u64> t1, u32 mask) const
      noexcept {
    auto t0v = t0.at(t0idx_);
    auto v =
        h::FastHash1{}.mix(TotalHashArgs).mix(index_).mix(BigramSeed).mix(t0v);
    return raw2(v.result(), t1, mask);
  }

  constexpr u32 target() const noexcept { return target_; }
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
    auto v =
        h::FastHash1{}.mix(TotalHashArgs).mix(index_).mix(TrigramSeed).mix(t0);
    state.at(target_) = v.result();
  }

  JPP_ALWAYS_INLINE void step1(util::ArraySlice<u64> patterns,
                               util::ArraySlice<u64> state,
                               util::MutableArraySlice<u64> result) const
      noexcept {
    auto t1 = patterns.at(t1idx_);
    auto s = state.at(target_);
    auto v = h::FastHash1{s}.mix(t1);
    result.at(target_) = v.result();
  }

  JPP_ALWAYS_INLINE u32 step2(util::ArraySlice<u64> patterns,
                              util::ArraySlice<u64> state,
                              util::MutableArraySlice<u32> result,
                              u32 mask) const noexcept {
    auto t2 = patterns.at(t2idx_);
    auto s = state.at(target_);
    auto v = h::FastHash1{s}.mix(t2);
    auto res = v.masked(mask);
    result.at(target_) = res;
    return res;
  }

  JPP_ALWAYS_INLINE u32 raw23(u64 state, util::ArraySlice<u64> t1,
                              util::ArraySlice<u64> t2, u32 mask) const
      noexcept {
    auto t1v = t1.at(t1idx_);
    auto t2v = t2.at(t2idx_);
    auto v = h::FastHash1{state}.mix(t1v).mix(t2v);
    return v.masked(mask);
  }

  JPP_ALWAYS_INLINE u32 jointRaw(util::ArraySlice<u64> t0,
                                 util::ArraySlice<u64> t1,
                                 util::ArraySlice<u64> t2, u32 mask) const
      noexcept {
    auto t0v = t0.at(t0idx_);
    auto v =
        h::FastHash1{}.mix(TotalHashArgs).mix(index_).mix(TrigramSeed).mix(t0v);
    return raw23(v.result(), t1, t2, mask);
  }

  JPP_ALWAYS_INLINE u32 jointApply(util::ArraySlice<u64> t0,
                                   util::ArraySlice<u64> t1,
                                   util::ArraySlice<u64> t2,
                                   util::MutableArraySlice<u32> result,
                                   u32 mask) const noexcept {
    u32 r = jointRaw(t0, t1, t2, mask);
    result.at(target_) = r;
    return r;
  }

  constexpr u32 target() const noexcept { return target_; }
};

template <typename Child>
class PartialNgramFeatureApplyImpl : public PartialNgramFeatureApply {
  const Child& child() const { return static_cast<const Child&>(*this); }

 public:
  void applyUni(FeatureBuffer* buffers, util::ConstSliceable<u64> p0,
                analysis::FeatureScorer* scorer,
                util::MutableArraySlice<float> result) const noexcept override {
    auto numUnigrams = child().numUnigrams();
    auto buf1 = buffers->valBuf1(numUnigrams);
    auto buf2 = buffers->valBuf2(numUnigrams);

    if (p0.numRows() == 0) {
      return;
    }

    auto weights = scorer->weights();
    auto mask = static_cast<u32>(weights.size() - 1);
    // do row 0
    child().uniStep0(p0.row(0), mask, weights, buf2);
    // do other rows
    u32 row = 1;
    for (; row < p0.numRows(); ++row) {
      auto patterns = p0.row(row);
      child().uniStep0(patterns, mask, weights, buf1);
      buf1.swap(buf2);
      result.at(row - 1) =
          analysis::impl::computeUnrolled4RawPerceptron(weights, buf1);
    }

    result.at(row - 1) =
        analysis::impl::computeUnrolled4RawPerceptron(weights, buf2);
  }

  void applyBiStep1(FeatureBuffer* buffers, util::ConstSliceable<u64> p0) const
      noexcept override {
    auto numElems = static_cast<u32>(p0.numRows());
    auto result = buffers->t1Buf(child().numBigrams(), numElems);
    for (auto row = 0; row < p0.numRows(); ++row) {
      auto patterns = p0.row(row);
      auto state = result.row(row);
      child().biStep0(patterns, state);
    }
    buffers->currentElems = numElems;
  }

  void applyBiStep2(FeatureBuffer* buffers, util::ArraySlice<u64> p1,
                    analysis::FeatureScorer* scorer,
                    util::MutableArraySlice<float> result) const
      noexcept override {
    auto numBigrams = child().numBigrams();
    auto buf1 = buffers->valBuf1(numBigrams);
    auto buf2 = buffers->valBuf2(numBigrams);
    auto numElems = buffers->currentElems;

    if (numElems == 0) {
      return;
    }

    auto state = buffers->t1Buf(numBigrams, numElems);

    auto weights = scorer->weights();
    auto mask = static_cast<u32>(weights.size() - 1);
    // do row 0
    child().biStep1(p1, state.row(0), mask, weights, buf2);
    // do other rows
    u32 row = 1;
    for (; row < state.numRows(); ++row) {
      auto srow = state.row(row);
      child().biStep1(p1, srow, mask, weights, buf1);
      buf1.swap(buf2);
      result.at(row - 1) +=
          analysis::impl::computeUnrolled4RawPerceptron(weights, buf1);
    }

    result.at(row - 1) +=
        analysis::impl::computeUnrolled4RawPerceptron(weights, buf2);
  }

  void applyTriStep1(FeatureBuffer* buffers, util::ConstSliceable<u64> p0) const
      noexcept override {
    auto numElems = static_cast<u32>(p0.numRows());
    JPP_DCHECK_EQ(numElems, buffers->currentElems);
    auto result = buffers->t2Buf1(child().numTrigrams(), numElems);
    for (auto row = 0; row < numElems; ++row) {
      auto patterns = p0.row(row);
      auto state = result.row(row);
      child().triStep0(patterns, state);
    }
    buffers->currentElems = numElems;
  }

  void applyTriStep2(FeatureBuffer* buffers, util::ArraySlice<u64> p1) const
      noexcept override {
    auto numElems = buffers->currentElems;
    auto state = buffers->t2Buf1(child().numTrigrams(), numElems);
    auto result = buffers->t2Buf2(child().numTrigrams(), numElems);
    for (auto row = 0; row < numElems; ++row) {
      auto st = state.row(row);
      auto res = result.row(row);
      child().triStep1(p1, st, res);
    }
    buffers->currentElems = numElems;
  }

  void applyTriStep3(FeatureBuffer* buffers, util::ArraySlice<u64> p2,
                     analysis::FeatureScorer* scorer,
                     util::MutableArraySlice<float> result) const
      noexcept override {
    auto numTrigrams = child().numTrigrams();
    auto buf1 = buffers->valBuf1(numTrigrams);
    auto buf2 = buffers->valBuf2(numTrigrams);
    auto numElems = buffers->currentElems;

    if (numElems == 0) {
      return;
    }

    auto state = buffers->t2Buf2(numTrigrams, numElems);

    auto weights = scorer->weights();
    auto mask = static_cast<u32>(weights.size() - 1);
    // do row 0
    child().triStep2(p2, state.row(0), mask, weights, buf2);
    // do other rows
    u32 row = 1;
    for (; row < state.numRows(); ++row) {
      auto srow = state.row(row);
      child().triStep2(p2, srow, mask, weights, buf1);
      buf1.swap(buf2);
      result.at(row - 1) +=
          analysis::impl::computeUnrolled4RawPerceptron(weights, buf1);
    }

    result.at(row - 1) +=
        analysis::impl::computeUnrolled4RawPerceptron(weights, buf2);
  }

  void applyBiTri(FeatureBuffer* buffers, u32 t0idx, util::ArraySlice<u64> t0,
                  util::ConstSliceable<u64> t1, util::ConstSliceable<u64> t2,
                  util::ArraySlice<u32> t1idxes,
                  analysis::FeatureScorer* scorer,
                  util::MutableArraySlice<float> result) const
      noexcept override {
    JPP_DCHECK_EQ(t1idxes.size(), t2.numRows());
    auto numElems = t2.numRows();
    if (numElems == 0) {
      return;
    }
    auto numBigrams = child().numBigrams();
    auto numTrigrams = child().numTrigrams();
    auto buf1 = buffers->valBuf1(numBigrams);
    auto buf2 = buffers->valBuf2(numBigrams);
    auto weights = scorer->weights();
    auto scbuf = buffers->scoreBuf(t1.numRows());
    JPP_DCHECK(util::memory::IsPowerOf2(weights.size()));
    u32 mask = static_cast<u32>(weights.size() - 1);

    child().biFull(t0, t1.row(0), mask, weights, buf2);
    u32 row = 1;
    for (; row < t1.numRows(); ++row) {
      auto t1v = t1.row(row);
      child().biFull(t0, t1v, mask, weights, buf1);
      buf1.swap(buf2);
      scbuf.at(row - 1) =
          analysis::impl::computeUnrolled4RawPerceptron(weights, buf1);
    }
    scbuf.at(row - 1) =
        analysis::impl::computeUnrolled4RawPerceptron(weights, buf2);

    for (int i = 0; i < t1idxes.size(); ++i) {
      result.at(i) = scbuf.at(t1idxes.at(i));
    }

    buf1 = buffers->valBuf1(numTrigrams);
    buf2 = buffers->valBuf2(numTrigrams);

    child().triFull(t0, t1.row(t1idxes.at(0)), t2.row(0), mask, weights, buf2);
    row = 1;
    for (; row < numElems; ++row) {
      auto t1v = t1.row(t1idxes.at(row));
      auto t2v = t2.row(row);
      child().triFull(t0, t1v, t2v, mask, weights, buf1);
      buf1.swap(buf2);
      result.at(row - 1) +=
          analysis::impl::computeUnrolled4RawPerceptron(weights, buf1);
    }
    result.at(row - 1) +=
        analysis::impl::computeUnrolled4RawPerceptron(weights, buf2);
  }
};

class PartialNgramDynamicFeatureApply
    : public PartialNgramFeatureApplyImpl<PartialNgramDynamicFeatureApply> {
  std::vector<UnigramFeature> unigrams_;
  std::vector<BigramFeature> bigrams_;
  std::vector<TrigramFeature> trigrams_;

 public:
  Status addChild(const spec::NgramFeatureDescriptor& nf);

  void uniStep0(util::ArraySlice<u64> patterns, u32 mask,
                analysis::WeightBuffer weights,
                util::MutableArraySlice<u32> result) const noexcept;

  void biStep0(util::ArraySlice<u64> patterns,
               util::MutableArraySlice<u64> state) const noexcept;

  void biStep1(util::ArraySlice<u64> patterns, util::ArraySlice<u64> state,
               u32 mask, analysis::WeightBuffer weights,
               util::MutableArraySlice<u32> result) const noexcept;

  void triStep0(util::ArraySlice<u64> patterns,
                util::MutableArraySlice<u64> state) const noexcept;

  void triStep1(util::ArraySlice<u64> patterns, util::ArraySlice<u64> state,
                util::MutableArraySlice<u64> result) const noexcept;

  void triStep2(util::ArraySlice<u64> patterns, util::ArraySlice<u64> state,
                u32 mask, analysis::WeightBuffer weights,
                util::MutableArraySlice<u32> result) const noexcept;

  void biFull(util::ArraySlice<u64> t0, util::ArraySlice<u64> t1, u32 mask,
              analysis::WeightBuffer weights,
              util::MutableArraySlice<u32> result) const noexcept;

  void triFull(util::ArraySlice<u64> t0, util::ArraySlice<u64> t1,
               util::ArraySlice<u64> t2, u32 mask,
               analysis::WeightBuffer weights,
               util::MutableArraySlice<u32> result) const noexcept;

  void allocateBuffers(FeatureBuffer* buffer, const AnalysisRunStats& stats,
                       util::memory::PoolAlloc* alloc) const override;

  u32 numUnigrams() const noexcept override {
    return static_cast<u32>(unigrams_.size());
  }
  u32 numBigrams() const noexcept override {
    return static_cast<u32>(bigrams_.size());
  }
  u32 numTrigrams() const noexcept override {
    return static_cast<u32>(trigrams_.size());
  }
};

}  // namespace impl
}  // namespace features
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_FEATURE_IMPL_NGRAM_PARTIAL_H
