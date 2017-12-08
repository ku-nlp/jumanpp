//
// Created by Arseny Tolmachev on 2017/11/07.
//

#ifndef JUMANPP_FEATURE_IMPL_PATTERN_H
#define JUMANPP_FEATURE_IMPL_PATTERN_H

#include "core/features_api.h"
#include "core/impl/feature_impl_compute.h"
#include "core/impl/feature_impl_prim.h"
#include "core/impl/feature_impl_types.h"
#include "util/array_slice.h"
#include "util/fast_hash.h"
#include "util/printer.h"

namespace jumanpp {
namespace core {
namespace features {
namespace impl {

using fh = util::hashing::FastHash1;

class DynamicPatternFeatureImpl {
  i32 index_;
  std::vector<ComputeFeatureImpl*> compute_;

 public:
  DynamicPatternFeatureImpl(i32 ind) : index_{ind} {}

  inline void apply(PrimitiveFeatureContext* pfc, const NodeInfo& info,
                    util::ArraySlice<i32> nodeFeatures,
                    util::MutableArraySlice<u64> result) const noexcept {
    auto u32idx = static_cast<u32>(index_);
    auto hash = fh{}.mix(u32idx).mix(compute_.size()).mix(PatternFeatureSeed);
    for (auto f : compute_) {
      hash = f->mixPrmitives(hash, pfc, info, nodeFeatures);
    }

    result.at(u32idx) = hash.result();
  }

  Status initialize(const PatternDynamicApplyImpl* parent,
                    const spec::PatternFeatureDescriptor& pfd);

  i32 index() const noexcept { return index_; }
};

class PatternDynamicApplyImpl final : public PatternFeatureApply {
  std::vector<std::unique_ptr<PrimitiveFeatureImpl>> primitive_;
  std::vector<std::unique_ptr<ComputeFeatureImpl>> compute_;
  std::vector<DynamicPatternFeatureImpl> patterns_;
  u32 uniOnlyFirst_ = 0;

  friend class DynamicPatternFeatureImpl;

 public:
  Status initialize(FeatureConstructionContext* ctx,
                    const spec::FeaturesSpec& spec);

  void apply(PrimitiveFeatureContext* pfc, const NodeInfo& info,
             util::ArraySlice<i32> nodeFeatures,
             util::MutableArraySlice<u64> result) const noexcept {
    for (auto& c : patterns_) {
      c.apply(pfc, info, nodeFeatures, result);
    }
  }

  void applyBatch(PrimitiveFeatureContext* pfc,
                  impl::PrimitiveFeatureData* data) const noexcept override {
    while (data->next()) {
      apply(pfc, data->nodeInfo(), data->entryData(), data->featureData());
    }
  }

  void applyUniOnly(PrimitiveFeatureContext* pfc, const NodeInfo& info,
                    util::ArraySlice<i32> nodeFeatures,
                    util::MutableArraySlice<u64> result) const noexcept {
    auto sz = patterns_.size();
    for (u32 patIdx = uniOnlyFirst_; patIdx < sz; ++patIdx) {
      auto& c = patterns_[patIdx];
      c.apply(pfc, info, nodeFeatures, result);
    }
  }

  void applyUniOnly(PrimitiveFeatureContext* pfc,
                    impl::PrimitiveFeatureData* data) const noexcept override {
    while (data->next()) {
      applyUniOnly(pfc, data->nodeInfo(), data->entryData(),
                   data->featureData());
    }
  }

  Status initPrimitive(
      FeatureConstructionContext* ctx,
      util::ArraySlice<spec::PrimitiveFeatureDescriptor> featureData);

  Status initCompute(
      util::ArraySlice<spec::ComputationFeatureDescriptor> vector);

  const PrimitiveFeatureImpl* primitive(i32 idx) const override {
    JPP_DCHECK_IN(idx, 0, primitive_.size());
    return primitive_[idx].get();
  }
};

}  // namespace impl
}  // namespace features
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_FEATURE_IMPL_PATTERN_H
