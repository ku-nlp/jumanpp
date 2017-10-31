//
// Created by Arseny Tolmachev on 2017/02/28.
//

#ifndef JUMANPP_FEATURE_IMPL_H
#define JUMANPP_FEATURE_IMPL_H

#include "core/core_types.h"
#include "core/dic/dictionary.h"
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

  virtual void apply(PrimitiveFeatureContext* ctx, const NodeInfo& nodeInfo,
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

  virtual void apply(PrimitiveFeatureContext* ctx, const NodeInfo& nodeInfo,
                     const util::ArraySlice<i32>& entry,
                     util::MutableArraySlice<u64>* features) const
      noexcept override {
    impl.apply(ctx, nodeInfo, entry, features);
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

  inline void apply(PrimitiveFeatureContext* ctx, const NodeInfo& nodeInfo,
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

  inline void apply(PrimitiveFeatureContext* ctx, const NodeInfo& nodeInfo,
                    const util::ArraySlice<i32>& entry,
                    util::MutableArraySlice<u64>* features) const noexcept {
    features->at(featureIdx) =
        (u32)ctx->providedFeature(nodeInfo.entryPtr(), providedIdx);
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

  inline void apply(PrimitiveFeatureContext* ctx, const NodeInfo& nodeInfo,
                    const util::ArraySlice<i32>& entry,
                    util::MutableArraySlice<u64>* features) const noexcept {
    auto fldPtr = entry[fieldIdx];
    auto length = ctx->lengthOf(nodeInfo, fieldIdx, fldPtr, field, true);
    JPP_DCHECK_NE(length, -1);
    features->at(featureIdx) = (u32)length;
  }
};

class CodepointLengthPrimFeatureImpl {
  u32 fieldIdx;
  u32 featureIdx;
  LengthFieldSource field = LengthFieldSource::Invalid;

 public:
  CodepointLengthPrimFeatureImpl() = default;
  constexpr CodepointLengthPrimFeatureImpl(u32 fieldIdx, u32 featureIdx,
                                           LengthFieldSource fld)
      : fieldIdx{fieldIdx}, featureIdx{featureIdx}, field{fld} {}

  Status initialize(FeatureConstructionContext* ctx, const PrimitiveFeature& f);

  inline void apply(PrimitiveFeatureContext* ctx, const NodeInfo& nodeInfo,
                    const util::ArraySlice<i32>& entry,
                    util::MutableArraySlice<u64>* features) const noexcept {
    auto fldPtr = entry[fieldIdx];
    auto length = ctx->lengthOf(nodeInfo, fieldIdx, fldPtr, field, false);
    JPP_DCHECK_NE(length, -1);
    features->at(featureIdx) = (u32)length;
  }
};

class CodepointFeatureImpl {
  u32 fieldIdx;
  i32 offset;

 public:
  CodepointFeatureImpl() = default;
  constexpr CodepointFeatureImpl(u32 fieldIdx, i32 offset)
      : fieldIdx{fieldIdx}, offset{offset} {}

  Status initialize(FeatureConstructionContext* ctx, const PrimitiveFeature& f);

  inline void apply(PrimitiveFeatureContext* ctx, const NodeInfo& nodeInfo,
                    const util::ArraySlice<i32>& entry,
                    util::MutableArraySlice<u64>* features) const noexcept {
    auto codepts = ctx->inputCodepoints();

    u64 value = ~u64{0};

    if (offset > 0) {
      auto off = offset - 1;
      auto pos = nodeInfo.end() + off;
      if (pos < codepts.size()) {
        value = codepts.at(pos).codepoint;
      }
    } else {
      auto pos = nodeInfo.start() + offset;
      if (0 <= pos && pos < codepts.size()) {
        value = codepts.at(pos).codepoint;
      }
    }

    features->at(fieldIdx) = value;
  }
};

class CodepointTypeFeatureImpl {
  u32 fieldIdx;
  i32 offset;

 public:
  CodepointTypeFeatureImpl() = default;
  constexpr CodepointTypeFeatureImpl(u32 fieldIdx, i32 offset)
      : fieldIdx{fieldIdx}, offset{offset} {}

  Status initialize(FeatureConstructionContext* ctx, const PrimitiveFeature& f);

  inline void apply(PrimitiveFeatureContext* ctx, const NodeInfo& nodeInfo,
                    const util::ArraySlice<i32>& entry,
                    util::MutableArraySlice<u64>* features) const noexcept {
    auto codepts = ctx->inputCodepoints();

    u64 value = 0;
    if (offset == 0) {
      for (int i = nodeInfo.start(); i < nodeInfo.end(); ++i) {
        auto cp = codepts.at(i);
        value |= static_cast<u32>(cp.charClass);
      }
    } else if (offset > 0) {
      auto off = offset - 1;
      auto pos = nodeInfo.end() + off;
      if (pos < codepts.size()) {
        value = static_cast<u32>(codepts.at(pos).charClass);
      }
    } else {
      auto pos = nodeInfo.start() + offset;
      if (0 <= pos && pos < codepts.size()) {
        value = static_cast<u32>(codepts.at(pos).charClass);
      }
    }

    features->at(fieldIdx) = value;
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

  inline void apply(PrimitiveFeatureContext* ctx, const NodeInfo& nodeInfo,
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

class MatchListElemPrimFeatureImpl {
  u32 fieldIdx;
  u32 featureIdx;
  util::ArraySlice<i32> matchData;

 public:
  MatchListElemPrimFeatureImpl() = default;
  template <size_t S>
  constexpr MatchListElemPrimFeatureImpl(u32 fieldIdx, u32 featureIdx,
                                         const i32 (&matchData)[S])
      : fieldIdx{fieldIdx}, featureIdx{featureIdx}, matchData{matchData} {}

  Status initialize(FeatureConstructionContext* ctx, const PrimitiveFeature& f);

  inline void apply(PrimitiveFeatureContext* ctx, const NodeInfo& nodeInfo,
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

class MatchKeyPrimFeatureImpl {
  u32 fieldIdx_;
  u32 featureIdx_;
  util::ArraySlice<i32> keys_;

 public:
  MatchKeyPrimFeatureImpl() = default;
  Status initialize(FeatureConstructionContext* ctx, const PrimitiveFeature& f);
  inline void apply(PrimitiveFeatureContext* ctx, const NodeInfo& nodeInfo,
                    const util::ArraySlice<i32>& entry,
                    util::MutableArraySlice<u64>* features) const noexcept {
    auto elem = entry.at(fieldIdx_);
    auto trav = ctx->kvTraversal(fieldIdx_, elem);
    i32 result = 0;
    while (trav.next()) {
      if (contains(keys_, trav.key())) {
        result = 1;
        break;
      }
    }
    features->at(featureIdx_) = (u32)result;
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
      cld.apply(ctx, data->nodeInfo(), data->entryData(), &slice);
    }
  }
};

class PrimitiveFeaturesDynamicApply final
    : public PrimitiveFeatureApplyImpl<PrimitiveFeaturesDynamicApply> {
  std::vector<std::unique_ptr<PrimitiveFeatureImpl>> features_;

 public:
  Status initialize(FeatureConstructionContext* ctx,
                    util::ArraySlice<PrimitiveFeature> featureData);

  void apply(PrimitiveFeatureContext* ctx, const NodeInfo& nodeInfo,
             const util::ArraySlice<i32>& entry,
             util::MutableArraySlice<u64>* features) const noexcept;
};

}  // namespace impl
}  // namespace features
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_FEATURE_IMPL_H
