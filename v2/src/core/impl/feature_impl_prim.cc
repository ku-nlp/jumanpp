//
// Created by Arseny Tolmachev on 2017/02/28.
//

#include "feature_impl_prim.h"

namespace jumanpp {
namespace core {
namespace features {
namespace impl {

Status CopyPrimFeatureImpl::initialize(FeatureConstructionContext *ctx,
                                       const PrimitiveFeature &f) {
  if (f.kind != PrimitiveFeatureKind::Copy) {
    return Status::InvalidParameter() << f.name << ": type was not Copy";
  }

  featureIdx = static_cast<u32>(f.index);

  if (f.references.size() != 1) {
    return Status::InvalidParameter()
           << f.name << ": number of parameters must be 1";
  }

  fieldIdx = static_cast<u32>(f.references.at(0));

  JPP_RETURN_IF_ERROR(ctx->checkFieldType(
      fieldIdx, {spec::ColumnType::String, spec::ColumnType::Int}));

  return Status::Ok();
}

Status ProvidedPrimFeatureImpl::initialize(FeatureConstructionContext *ctx,
                                           const PrimitiveFeature &f) {
  if (f.kind != PrimitiveFeatureKind::Provided) {
    return Status::InvalidParameter() << f.name << ": type was not Provided";
  }

  featureIdx = static_cast<u32>(f.index);
  if (f.references.size() != 1) {
    return Status::InvalidParameter()
           << f.name << ": number of parameters must be 1";
  }

  providedIdx = static_cast<u32>(f.references.at(0));

  JPP_RETURN_IF_ERROR(ctx->checkProvidedFeature(providedIdx));

  return Status::Ok();
}

Status LengthPrimFeatureImpl::initialize(FeatureConstructionContext *ctx,
                                         const PrimitiveFeature &f) {
  if (f.kind != PrimitiveFeatureKind::Length) {
    return Status::InvalidParameter() << f.name << ": type was not Length";
  }

  featureIdx = static_cast<u32>(f.index);

  if (f.references.size() != 1) {
    return Status::InvalidParameter()
           << f.name << ": number of parameters must be 1";
  }

  fieldIdx = static_cast<u32>(f.references.at(0));

  JPP_RETURN_IF_ERROR(ctx->setLengthField(fieldIdx, &field));

  return Status::Ok();
}

Status CodepointLengthPrimFeatureImpl::initialize(
    FeatureConstructionContext *ctx, const PrimitiveFeature &f) {
  if (f.kind != PrimitiveFeatureKind::CodepointSize) {
    return Status::InvalidParameter()
           << f.name << ": type was not CodepointSize";
  }

  featureIdx = static_cast<u32>(f.index);

  if (f.references.size() != 1) {
    return Status::InvalidParameter()
           << f.name << ": number of parameters must be 1";
  }

  fieldIdx = static_cast<u32>(f.references.at(0));

  JPP_RETURN_IF_ERROR(ctx->setLengthField(fieldIdx, &field));

  return Status::Ok();
}

Status MatchDicPrimFeatureImpl::initialize(FeatureConstructionContext *ctx,
                                           const PrimitiveFeature &f) {
  if (f.kind != PrimitiveFeatureKind::MatchDic) {
    return Status::InvalidParameter() << f.name << ": type was not MatchDic";
  }

  featureIdx = static_cast<u32>(f.index);

  if (f.references.size() != 1) {
    return Status::InvalidParameter()
           << f.name << ": number of parameters must be 1";
  }

  fieldIdx = static_cast<u32>(f.references.at(0));

  JPP_RETURN_IF_ERROR(ctx->checkFieldType(
      fieldIdx, {spec::ColumnType::String, spec::ColumnType::Int}));

  matchData = f.matchData;

  return Status::Ok();
}

Status MatchListElemPrimFeatureImpl::initialize(FeatureConstructionContext *ctx,
                                                const PrimitiveFeature &f) {
  if (f.kind != PrimitiveFeatureKind::MatchListElem) {
    return Status::InvalidParameter()
           << f.name << ": type was not MatchListElem";
  }

  featureIdx = static_cast<u32>(f.index);

  if (f.references.size() != 1) {
    return Status::InvalidParameter()
           << f.name << ": number of parameters must be 1, was "
           << f.references.size();
  }

  fieldIdx = static_cast<u32>(f.references.at(0));

  JPP_RETURN_IF_ERROR(
      ctx->checkFieldType(fieldIdx, {spec::ColumnType::StringList}));

  matchData = f.matchData;

  return Status::Ok();
}

Status MatchKeyPrimFeatureImpl::initialize(FeatureConstructionContext *ctx,
                                           const PrimitiveFeature &f) {
  if (f.kind != PrimitiveFeatureKind::MatchKey) {
    return Status::InvalidParameter() << f.name << ": type was not MatchKey";
  }

  featureIdx_ = static_cast<u32>(f.index);

  if (f.references.size() != 1) {
    return JPPS_INVALID_PARAMETER << f.name
                                  << ": number of parameters must be 1, was "
                                  << f.references.size();
  }

  fieldIdx_ = static_cast<u32>(f.references[0]);

  JPP_RETURN_IF_ERROR(
      ctx->checkFieldType(fieldIdx_, {spec::ColumnType::StringKVList}));

  keys_ = f.matchData;

  return Status::Ok();
}

Status FeatureConstructionContext::checkFieldType(
    i32 field, std::initializer_list<spec::ColumnType> columnTypes) const {
  auto &fld = fields->at(field);

  for (auto tp : columnTypes) {
    if (tp == fld.columnType) {
      return Status::Ok();
    }
  }

  auto status = JPPS_INVALID_PARAMETER;
  status << "dic field " << fld.name << " had type " << fld.columnType
         << " , allowed=[";
  for (auto tp : columnTypes) {
    status << tp << " ";
  }

  status << "]";

  return status;
}

FeatureConstructionContext::FeatureConstructionContext(
    const dic::FieldsHolder *fields)
    : fields(fields) {}

Status PrimitiveFeaturesDynamicApply::initialize(
    FeatureConstructionContext *ctx,
    util::ArraySlice<PrimitiveFeature> featureData) {
  features_.reserve(featureData.size());
  for (auto &f : featureData) {
    std::unique_ptr<PrimitiveFeatureImpl> feature;
    switch (f.kind) {
      case PrimitiveFeatureKind::Invalid:
        return JPPS_INVALID_PARAMETER << "uninitialized feature " << f.name;
      case PrimitiveFeatureKind::Copy:
        feature.reset(new DynamicPrimitiveFeature<CopyPrimFeatureImpl>{});
        break;
      case PrimitiveFeatureKind::Provided:
        feature.reset(new DynamicPrimitiveFeature<ProvidedPrimFeatureImpl>{});
        break;
      case PrimitiveFeatureKind::MatchDic:
        feature.reset(new DynamicPrimitiveFeature<MatchDicPrimFeatureImpl>{});
        break;
      case PrimitiveFeatureKind::MatchListElem:
        feature.reset(
            new DynamicPrimitiveFeature<MatchListElemPrimFeatureImpl>{});
        break;
      case PrimitiveFeatureKind::Length:
        feature.reset(new DynamicPrimitiveFeature<LengthPrimFeatureImpl>{});
        break;
      case PrimitiveFeatureKind::CodepointSize:
        feature.reset(
            new DynamicPrimitiveFeature<CodepointLengthPrimFeatureImpl>{});
        break;
      case PrimitiveFeatureKind::MatchKey:
        feature.reset(new DynamicPrimitiveFeature<MatchKeyPrimFeatureImpl>{});
        break;
      case PrimitiveFeatureKind::Codepoint:
        feature.reset(new DynamicPrimitiveFeature<CodepointFeatureImpl>{});
        break;
      case PrimitiveFeatureKind::CodepointType:
        feature.reset(new DynamicPrimitiveFeature<CodepointTypeFeatureImpl>{});
        break;
      default:
        return JPPS_NOT_IMPLEMENTED << "Could not create feature: " << f.name
                                    << " its type was not supported";
    }
    JPP_RIE_MSG(feature->initialize(ctx, f),
                "failed to initialize feature: " << f.name);
    features_.emplace_back(std::move(feature));
  }
  return Status::Ok();
}

void PrimitiveFeaturesDynamicApply::apply(
    PrimitiveFeatureContext *ctx, const NodeInfo &nodeInfo,
    const util::ArraySlice<i32> &entry,
    util::MutableArraySlice<u64> *features) const noexcept {
  for (auto &f : features_) {
    f->apply(ctx, nodeInfo, entry, features);
  }
}

Status CodepointFeatureImpl::initialize(FeatureConstructionContext *ctx,
                                        const PrimitiveFeature &f) {
  if (f.matchData.size() != 1) {
    return JPPS_INVALID_PARAMETER << "match data of feature [" << f.name
                                  << "] should be 1, was "
                                  << f.matchData.size();
  }
  this->fieldIdx = static_cast<u32>(f.index);
  this->offset = f.matchData[0];
  return Status::Ok();
}

Status CodepointTypeFeatureImpl::initialize(FeatureConstructionContext *ctx,
                                            const PrimitiveFeature &f) {
  if (f.matchData.size() != 1) {
    return JPPS_INVALID_PARAMETER << "match data of feature [" << f.name
                                  << "] should be 1, was "
                                  << f.matchData.size();
  }
  this->fieldIdx = static_cast<u32>(f.index);
  this->offset = f.matchData[0];
  return Status::Ok();
}

}  // namespace impl
}  // namespace features
}  // namespace core
}  // namespace jumanpp