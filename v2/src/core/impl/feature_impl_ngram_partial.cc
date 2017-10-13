//
// Created by Arseny Tolmachev on 2017/10/13.
//

#include "feature_impl_ngram_partial.h"

namespace jumanpp {
namespace core {
namespace features {
namespace impl {

Status PartialNgramDynamicFeatureApply::addChild(const NgramFeature& nf) {
  auto& args = nf.arguments;
  auto sz = args.size();
  switch (sz) {
    case 1: {
      unigrams_.emplace_back(unigrams_.size(), args[0]);
      break;
    }
    case 2: {
      bigrams_.emplace_back(bigrams_.size(), args[0], args[1]);
      break;
    }
    case 3: {
      trigrams_.emplace_back(trigrams_.size(), args[0], args[1], args[2]);
      break;
    }
    default:
      return JPPS_INVALID_STATE << "invalid ngram feature #" << nf.index
                                << "of order " << nf.arguments.size()
                                << " only 1-3 are supported";
  }
  return Status::Ok();
}

void PartialNgramDynamicFeatureApply::uniStep0(
    util::ArraySlice<u64> patterns, util::MutableArraySlice<u32> result) const
    noexcept {
  for (auto& uni : unigrams_) {
    uni.step0(patterns, result);
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
    util::ArraySlice<u64> patterns, util::ArraySlice<u64> state,
    util::MutableArraySlice<u32> result) const noexcept {
  for (auto& bi : bigrams_) {
    bi.step1(patterns, state, result);
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
    util::ArraySlice<u64> patterns, util::ArraySlice<u64> state,
    util::MutableArraySlice<u32> result) const noexcept {
  for (auto& tri : trigrams_) {
    tri.step2(patterns, state, result);
  }
}

}  // namespace impl
}  // namespace features
}  // namespace core
}  // namespace jumanpp