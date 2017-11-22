//
// Created by Arseny Tolmachev on 2017/11/07.
//

#include "feature_impl_pattern.h"

namespace jumanpp {
namespace core {
namespace features {
namespace impl {

Status DynamicPatternFeatureImpl::initialize(
    const PatternDynamicApplyImpl *parent,
    const spec::PatternFeatureDescriptor &pfd) {
  index_ = pfd.index;
  auto &computeFeatures = parent->compute_;
  for (auto ref : pfd.references) {
    if (ref < 0 || ref > computeFeatures.size()) {
      return JPPS_INVALID_PARAMETER
             << "compute feature is out of bound: " << ref << " for pattern "
             << index_;
    }
    compute_.push_back(computeFeatures[ref].get());
  }

  return Status::Ok();
}

Status PatternDynamicApplyImpl::initialize(FeatureConstructionContext *ctx,
                                           const spec::FeaturesSpec &spec) {
  JPP_RETURN_IF_ERROR(initPrimitive(ctx, spec.primitive));
  JPP_RETURN_IF_ERROR(initCompute(spec.computation));
  for (auto &f : spec.pattern) {
    patterns_.emplace_back(f.index);
    auto &fi = patterns_.back();
    JPP_CAPTURE(f.index);
    JPP_RETURN_IF_ERROR(fi.initialize(this, f));
  }
  uniOnlyFirst_ = static_cast<u32>(patterns_.size() - spec.numUniOnlyPats);
  JPP_DCHECK_LE(uniOnlyFirst_, patterns_.size());
  return Status::Ok();
}

Status PatternDynamicApplyImpl::initPrimitive(
    FeatureConstructionContext *ctx,
    util::ArraySlice<spec::PrimitiveFeatureDescriptor> featureData) {
  primitive_.reserve(featureData.size());
  for (auto &f : featureData) {
    std::unique_ptr<PrimitiveFeatureImpl> feature;
    switch (f.kind) {
      case PrimitiveFeatureKind::Invalid:
        return JPPS_INVALID_PARAMETER << "uninitialized feature " << f.name;
      case PrimitiveFeatureKind::Copy:
        feature.reset(
            new DynamicPrimitiveFeature<CopyPrimFeatureImpl>{f.index});
        break;
      case PrimitiveFeatureKind::SingleBit:
        feature.reset(
            new DynamicPrimitiveFeature<ShiftMaskPrimFeatureImpl>{f.index});
        break;
      case PrimitiveFeatureKind::Provided:
        feature.reset(
            new DynamicPrimitiveFeature<ProvidedPrimFeatureImpl>{f.index});
        break;
      case PrimitiveFeatureKind::ByteLength:
        feature.reset(
            new DynamicPrimitiveFeature<ByteLengthPrimFeatureImpl>{f.index});
        break;
      case PrimitiveFeatureKind::CodepointSize:
        feature.reset(
            new DynamicPrimitiveFeature<CodepointLengthPrimFeatureImpl>{
                f.index});
        break;
      case PrimitiveFeatureKind::SurfaceCodepointSize:
        feature.reset(
            new DynamicPrimitiveFeature<SurfaceCodepointLengthPrimFeatureImpl>{
                f.index});
        break;
      case PrimitiveFeatureKind::Codepoint:
        feature.reset(
            new DynamicPrimitiveFeature<CodepointFeatureImpl>{f.index});
        break;
      case PrimitiveFeatureKind::CodepointType:
        feature.reset(
            new DynamicPrimitiveFeature<CodepointTypeFeatureImpl>{f.index});
        break;
      default:
        return JPPS_NOT_IMPLEMENTED << "Could not create feature: " << f.name
                                    << " its type was not supported";
    }
    JPP_RIE_MSG(feature->initialize(ctx, f),
                "failed to initialize feature: " << f.name);
    primitive_.emplace_back(std::move(feature));
  }
  return Status::Ok();
}

Status PatternDynamicApplyImpl::initCompute(
    util::ArraySlice<spec::ComputationFeatureDescriptor> data) {
  for (auto &nfo : data) {
    std::unique_ptr<ComputeFeatureImpl> ptr;
    if (nfo.falseBranch.empty() && nfo.trueBranch.empty()) {
      // noop
      ptr.reset(new NoopComputeFeatureImpl);
    } else {
      ptr.reset(new ExprComputeFeatureImpl);
    }
    JPP_RIE_MSG(ptr->initialize(nfo, primitive_, compute_),
                "for pattern feature: " << nfo.name);
    compute_.emplace_back(std::move(ptr));
  }

  return Status::Ok();
}

}  // namespace impl
}  // namespace features
}  // namespace core
}  // namespace jumanpp