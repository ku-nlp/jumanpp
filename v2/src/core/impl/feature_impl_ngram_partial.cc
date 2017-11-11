//
// Created by Arseny Tolmachev on 2017/10/13.
//

#include "feature_impl_ngram_partial.h"

namespace jumanpp {
namespace core {
namespace features {
namespace impl {

Status PartialNgramDynamicFeatureApply::addChild(
    const spec::NgramFeatureDescriptor& nf) {
  auto& args = nf.references;
  auto sz = args.size();
  switch (sz) {
    case 1: {
      unigrams_.emplace_back(unigrams_.size(), nf.index, args[0]);
      break;
    }
    case 2: {
      bigrams_.emplace_back(bigrams_.size(), nf.index, args[0], args[1]);
      break;
    }
    case 3: {
      trigrams_.emplace_back(trigrams_.size(), nf.index, args[0], args[1],
                             args[2]);
      break;
    }
    default:
      return JPPS_INVALID_STATE << "invalid ngram feature #" << nf.index
                                << "of order " << args.size()
                                << " only 1-3 are supported";
  }
  return Status::Ok();
}

void PartialNgramDynamicFeatureApply::uniStep0(
    util::ArraySlice<u64> patterns, u32 mask, analysis::WeightBuffer weights,
    util::MutableArraySlice<u32> result) const noexcept {
  for (auto& uni : unigrams_) {
    auto idx = uni.step0(patterns, result, mask);
    weights.prefetch<util::PrefetchHint::PREFETCH_HINT_T2>(idx);
  }
}

void PartialNgramDynamicFeatureApply::biStep0(
    util::ArraySlice<u64> patterns, util::MutableArraySlice<u64> state) const
    noexcept {
  for (auto& bi : bigrams_) {
    bi.step0(patterns, state);
  }
}

void PartialNgramDynamicFeatureApply::biStep1(
    util::ArraySlice<u64> patterns, util::ArraySlice<u64> state, u32 mask,
    analysis::WeightBuffer weights, util::MutableArraySlice<u32> result) const
    noexcept {
  for (auto& bi : bigrams_) {
    auto idx = bi.step1(patterns, state, result, mask);
    weights.prefetch<util::PrefetchHint::PREFETCH_HINT_T2>(idx);
  }
}

void PartialNgramDynamicFeatureApply::triStep0(
    util::ArraySlice<u64> patterns, util::MutableArraySlice<u64> state) const
    noexcept {
  for (auto& tri : trigrams_) {
    tri.step0(patterns, state);
  }
}

void PartialNgramDynamicFeatureApply::triStep1(
    util::ArraySlice<u64> patterns, util::ArraySlice<u64> state,
    util::MutableArraySlice<u64> result) const noexcept {
  for (auto& tri : trigrams_) {
    tri.step1(patterns, state, result);
  }
}

void PartialNgramDynamicFeatureApply::triStep2(
    util::ArraySlice<u64> patterns, util::ArraySlice<u64> state, u32 mask,
    analysis::WeightBuffer weights, util::MutableArraySlice<u32> result) const
    noexcept {
  for (auto& tri : trigrams_) {
    auto idx = tri.step2(patterns, state, result, mask);
    weights.prefetch<util::PrefetchHint::PREFETCH_HINT_T2>(idx);
  }
}

void PartialNgramDynamicFeatureApply::biFull(
    util::ArraySlice<u64> t0, util::ArraySlice<u64> t1, u32 mask,
    analysis::WeightBuffer weights, util::MutableArraySlice<u32> result) const
    noexcept {
  for (auto& bi : bigrams_) {
    auto idx = bi.jointApply(t0, t1, result, mask);
    weights.prefetch<util::PrefetchHint::PREFETCH_HINT_T2>(idx);
  }
}

void PartialNgramDynamicFeatureApply::triFull(
    util::ArraySlice<u64> t0, util::ArraySlice<u64> t1,
    util::ArraySlice<u64> t2, u32 mask, analysis::WeightBuffer weights,
    util::MutableArraySlice<u32> result) const noexcept {
  for (auto& tri : trigrams_) {
    auto idx = tri.jointApply(t0, t1, t2, result, mask);
    weights.prefetch<util::PrefetchHint::PREFETCH_HINT_T2>(idx);
  }
}

void PartialNgramDynamicFeatureApply::allocateBuffers(
    FeatureBuffer* buffer, const AnalysisRunStats& stats,
    util::memory::PoolAlloc* alloc) const {
  u32 maxNgrams = std::max({numUnigrams(), numBigrams(), numTrigrams()});
  buffer->currentElems = ~0u;
  buffer->valueBuffer1 = alloc->allocateBuf<u32>(maxNgrams, 64);
  buffer->valueBuffer2 = alloc->allocateBuf<u32>(maxNgrams, 64);
  buffer->t1Buffer =
      alloc->allocateBuf<u64>(numBigrams() * stats.maxStarts, 64);
  buffer->t2Buffer1 =
      alloc->allocateBuf<u64>(numTrigrams() * stats.maxStarts, 64);
  buffer->t2Buffer2 =
      alloc->allocateBuf<u64>(numTrigrams() * stats.maxStarts, 64);
  buffer->scoreBuffer = alloc->allocateBuf<float>(stats.maxEnds, 16);
}

}  // namespace impl
}  // namespace features
}  // namespace core
}  // namespace jumanpp