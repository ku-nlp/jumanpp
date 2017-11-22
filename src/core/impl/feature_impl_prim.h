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
                            const spec::PrimitiveFeatureDescriptor& f) = 0;

  virtual u64 access(PrimitiveFeatureContext* ctx, const NodeInfo& nodeInfo,
                     const util::ArraySlice<i32>& entry) const = 0;

  virtual i32 index() const = 0;
};

template <typename Impl>
class DynamicPrimitiveFeature : public PrimitiveFeatureImpl {
  Impl impl;
  i32 index_;

 public:
  explicit DynamicPrimitiveFeature(i32 index) : index_{index} {}

  virtual Status initialize(
      FeatureConstructionContext* ctx,
      const spec::PrimitiveFeatureDescriptor& f) override {
    return impl.initialize(ctx, f);
  }

  virtual u64 access(PrimitiveFeatureContext* ctx, const NodeInfo& nodeInfo,
                     const util::ArraySlice<i32>& entry) const override {
    return impl.access(ctx, nodeInfo, entry);
  }

  virtual i32 index() const override { return index_; }
};

class CopyPrimFeatureImpl {
  u32 fieldIdx;

 public:
  CopyPrimFeatureImpl() = default;
  constexpr CopyPrimFeatureImpl(u32 fieldIdx) : fieldIdx{fieldIdx} {}

  Status initialize(FeatureConstructionContext* ctx,
                    const spec::PrimitiveFeatureDescriptor& f);

  inline u64 access(PrimitiveFeatureContext* ctx, const NodeInfo& nodeInfo,
                    const util::ArraySlice<i32>& entry) const {
    return static_cast<u32>(entry.at(fieldIdx));
  }
};

class ShiftMaskPrimFeatureImpl {
  u32 fieldIdx_;
  u32 shift_;
  u32 mask_;

 public:
  ShiftMaskPrimFeatureImpl() = default;
  constexpr ShiftMaskPrimFeatureImpl(u32 fieldIdx_, u32 shift, u32 mask)
      : fieldIdx_{fieldIdx_}, shift_{shift}, mask_{mask} {}

  Status initialize(FeatureConstructionContext* ctx,
                    const spec::PrimitiveFeatureDescriptor& f);

  inline u64 access(PrimitiveFeatureContext* ctx, const NodeInfo& nodeInfo,
                    const util::ArraySlice<i32>& entry) const {
    return (static_cast<u32>(entry.at(fieldIdx_)) >> shift_) & mask_;
  }
};

class ProvidedPrimFeatureImpl {
  u32 providedIdx;

 public:
  ProvidedPrimFeatureImpl() {}
  constexpr ProvidedPrimFeatureImpl(u32 providedIdx)
      : providedIdx(providedIdx) {}

  Status initialize(FeatureConstructionContext* ctx,
                    const spec::PrimitiveFeatureDescriptor& f);

  inline u64 access(PrimitiveFeatureContext* ctx, const NodeInfo& nodeInfo,
                    const util::ArraySlice<i32>& entry) const {
    return (u32)ctx->providedFeature(nodeInfo.entryPtr(), providedIdx);
  }
};

class ByteLengthPrimFeatureImpl {
  u32 fieldIdx;
  LengthFieldSource field = LengthFieldSource::Invalid;

 public:
  ByteLengthPrimFeatureImpl() {}
  constexpr ByteLengthPrimFeatureImpl(u32 fieldIdx, LengthFieldSource fld)
      : fieldIdx{fieldIdx}, field{fld} {}

  Status initialize(FeatureConstructionContext* ctx,
                    const spec::PrimitiveFeatureDescriptor& f);

  inline u64 access(PrimitiveFeatureContext* ctx, const NodeInfo& nodeInfo,
                    const util::ArraySlice<i32>& entry) const {
    auto fldPtr = entry[fieldIdx];
    auto length = ctx->lengthOf(nodeInfo, fieldIdx, fldPtr, field, true);
    JPP_DCHECK_NE(length, -1);
    return static_cast<u64>(length);
  }
};

class CodepointLengthPrimFeatureImpl {
  u32 fieldIdx;
  LengthFieldSource field = LengthFieldSource::Invalid;

 public:
  CodepointLengthPrimFeatureImpl() = default;
  constexpr CodepointLengthPrimFeatureImpl(u32 fieldIdx, LengthFieldSource fld)
      : fieldIdx{fieldIdx}, field{fld} {}

  Status initialize(FeatureConstructionContext* ctx,
                    const spec::PrimitiveFeatureDescriptor& f);

  inline u64 access(PrimitiveFeatureContext* ctx, const NodeInfo& nodeInfo,
                    const util::ArraySlice<i32>& entry) const {
    auto fldPtr = entry[fieldIdx];
    auto length = ctx->lengthOf(nodeInfo, fieldIdx, fldPtr, field, false);
    JPP_DCHECK_NE(length, -1);
    return static_cast<u64>(length);
  }
};

class SurfaceCodepointLengthPrimFeatureImpl {
 public:
  constexpr SurfaceCodepointLengthPrimFeatureImpl() = default;

  Status initialize(FeatureConstructionContext* ctx,
                    const spec::PrimitiveFeatureDescriptor& f);

  inline u64 access(PrimitiveFeatureContext* ctx, const NodeInfo& nodeInfo,
                    const util::ArraySlice<i32>& entry) const {
    auto length = nodeInfo.end() - nodeInfo.start();
    return static_cast<u64>(length);
  }
};

class CodepointFeatureImpl {
  i32 offset;

 public:
  CodepointFeatureImpl() = default;
  constexpr CodepointFeatureImpl(i32 offset) : offset{offset} {}

  Status initialize(FeatureConstructionContext* ctx,
                    const spec::PrimitiveFeatureDescriptor& f);

  inline u64 access(PrimitiveFeatureContext* ctx, const NodeInfo& nodeInfo,
                    const util::ArraySlice<i32>& entry) const {
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
    return value;
  }
};

class CodepointTypeFeatureImpl {
  i32 offset;

 public:
  CodepointTypeFeatureImpl() = default;
  constexpr CodepointTypeFeatureImpl(i32 offset) : offset{offset} {}

  Status initialize(FeatureConstructionContext* ctx,
                    const spec::PrimitiveFeatureDescriptor& f);

  inline u64 access(PrimitiveFeatureContext* ctx, const NodeInfo& nodeInfo,
                    const util::ArraySlice<i32>& entry) const {
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
    return value;
  }
};

}  // namespace impl
}  // namespace features
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_FEATURE_IMPL_H
