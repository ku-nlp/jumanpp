//
// Created by Arseny Tolmachev on 2017/02/28.
//

#ifndef JUMANPP_FEATURE_IMPL_H
#define JUMANPP_FEATURE_IMPL_H

#include "core/core_types.h"
#include "core/dictionary.h"
#include "core/features_api.h"
#include "core/impl/feature_impl_types.h"
#include "core/impl/feature_types.h"
#include "util/array_slice.h"
#include "util/stl_util.h"
#include "util/string_piece.h"
#include "util/types.hpp"

namespace jumanpp {
namespace core {
namespace features {
namespace impl {

class FeatureImplBase {
 public:
  constexpr FeatureImplBase() {}
  virtual ~FeatureImplBase() = default;
};

class PrimitiveFeatureImpl : public FeatureImplBase {
 public:
  virtual Status initialize(FeatureConstructionContext* ctx,
                            const PrimitiveFeature& f) = 0;

  virtual void apply(PrimitiveFeatureContext* ctx, EntryPtr entryPtr,
                     const util::ArraySlice<i32>& entry,
                     util::MutableArraySlice<u64>* features) const noexcept = 0;
};

template <typename Impl>
class DynamicPrimitiveFeature : public PrimitiveFeatureImpl {
  Impl impl;

 public:
  virtual Status initialize(FeatureConstructionContext* ctx,
                            const PrimitiveFeature& f) override {
    return impl.initialize(ctx, f);
  }

  virtual void apply(PrimitiveFeatureContext* ctx, EntryPtr entryPtr,
                     const util::ArraySlice<i32>& entry,
                     util::MutableArraySlice<u64>* features) const
      noexcept override {
    impl.apply(ctx, entryPtr, entry, features);
  }
};

class CopyPrimFeatureImpl {
  u32 fieldIdx;
  u32 featureIdx;

 public:
  CopyPrimFeatureImpl() {}
  constexpr CopyPrimFeatureImpl(u32 fieldIdx, u32 featureIdx)
      : fieldIdx{fieldIdx}, featureIdx{featureIdx} {}

  Status initialize(FeatureConstructionContext* ctx, const PrimitiveFeature& f);

  inline void apply(PrimitiveFeatureContext* ctx, EntryPtr entryPtr,
                    const util::ArraySlice<i32>& entry,
                    util::MutableArraySlice<u64>* features) const noexcept {
    features->at(featureIdx) = (u32)entry.at(fieldIdx);
  }
};

class ProvidedPrimFeatureImpl {
  u32 providedIdx;
  u32 featureIdx;

 public:
  ProvidedPrimFeatureImpl() {}
  constexpr ProvidedPrimFeatureImpl(u32 providedIdx, u32 featureIdx)
      : providedIdx(providedIdx), featureIdx(featureIdx) {}

  Status initialize(FeatureConstructionContext* ctx, const PrimitiveFeature& f);

  inline void apply(PrimitiveFeatureContext* ctx, EntryPtr entryPtr,
                    const util::ArraySlice<i32>& entry,
                    util::MutableArraySlice<u64>* features) const noexcept {
    features->at(featureIdx) = (u32)ctx->providedFeature(entryPtr, providedIdx);
  }
};

class LengthPrimFeatureImpl {
  u32 fieldIdx;
  u32 featureIdx;
  LengthFieldSource field = LengthFieldSource::Invalid;

 public:
  LengthPrimFeatureImpl() {}
  constexpr LengthPrimFeatureImpl(u32 fieldIdx, u32 featureIdx,
                                  LengthFieldSource fld)
      : fieldIdx{fieldIdx}, featureIdx{featureIdx}, field{fld} {}

  Status initialize(FeatureConstructionContext* ctx, const PrimitiveFeature& f);

  inline void apply(PrimitiveFeatureContext* ctx, EntryPtr entryPtr,
                    const util::ArraySlice<i32>& entry,
                    util::MutableArraySlice<u64>* features) const noexcept {
    auto fldPtr = entry[fieldIdx];
    auto length = ctx->lengthOf(fieldIdx, fldPtr, field);
    JPP_DCHECK_NE(length, -1);
    features->at(featureIdx) = (u32)length;
  }
};

class MatchDicPrimFeatureImpl {
  u32 fieldIdx;
  u32 featureIdx;
  util::ArraySlice<i32> matchData;

 public:
  MatchDicPrimFeatureImpl() {}
  template <size_t S>
  constexpr MatchDicPrimFeatureImpl(u32 fieldIdx, u32 featureIdx,
                                    const i32 (&matchData)[S])
      : fieldIdx{fieldIdx}, featureIdx{featureIdx}, matchData{matchData} {}

  Status initialize(FeatureConstructionContext* ctx, const PrimitiveFeature& f);

  inline void apply(PrimitiveFeatureContext* ctx, EntryPtr entryPtr,
                    const util::ArraySlice<i32>& entry,
                    util::MutableArraySlice<u64>* features) const noexcept {
    auto elem = entry.at(fieldIdx);
    i32 result = 0;
    if (contains(matchData, elem)) {
      result = 1;
    }
    features->at(featureIdx) = (u32)result;
  }
};

class MatchAnyDicPrimFeatureImpl {
  u32 fieldIdx;
  u32 featureIdx;
  util::ArraySlice<i32> matchData;

 public:
  MatchAnyDicPrimFeatureImpl() {}
  template <size_t S>
  constexpr MatchAnyDicPrimFeatureImpl(u32 fieldIdx, u32 featureIdx,
                                       const i32 (&matchData)[S])
      : fieldIdx{fieldIdx}, featureIdx{featureIdx}, matchData{matchData} {}

  Status initialize(FeatureConstructionContext* ctx, const PrimitiveFeature& f);

  inline void apply(PrimitiveFeatureContext* ctx, EntryPtr entryPtr,
                    const util::ArraySlice<i32>& entry,
                    util::MutableArraySlice<u64>* features) const noexcept {
    auto elem = entry.at(fieldIdx);
    auto trav = ctx->traversal(fieldIdx, elem);
    i32 result = 0;
    i32 value = 0;
    while (trav.next(&value)) {
      if (contains(matchData, value)) {
        result = 1;
        break;
      }
    }
    features->at(featureIdx) = (u32)result;
  }
};

template <typename Child>
class PrimitiveFeatureApplyImpl : public PrimitiveFeatureApply {
 public:
  virtual void applyBatch(impl::PrimitiveFeatureContext* ctx,
                          impl::PrimitiveFeatureData* data) const
      noexcept override {
    const Child& cld = static_cast<const Child&>(*this);
    while (data->next()) {
      auto slice = data->featureData();
      cld.apply(ctx, data->entry(), data->entryData(), &slice);
    }
  }
};

class PrimitiveFeaturesDynamicApply final
    : public PrimitiveFeatureApplyImpl<PrimitiveFeaturesDynamicApply> {
  std::vector<std::unique_ptr<PrimitiveFeatureImpl>> features_;

 public:
  Status initialize(FeatureConstructionContext* ctx,
                    util::ArraySlice<PrimitiveFeature> featureData);

  void apply(PrimitiveFeatureContext* ctx, EntryPtr entryPtr,
             const util::ArraySlice<i32>& entry,
             util::MutableArraySlice<u64>* features) const noexcept;
};

}  // impl
}  // features
}  // core
}  // jumanpp

#endif  // JUMANPP_FEATURE_IMPL_H
