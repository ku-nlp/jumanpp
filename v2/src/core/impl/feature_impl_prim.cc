//
// Created by Arseny Tolmachev on 2017/02/28.
//

#include "feature_impl_prim.h"

namespace jumanpp {
namespace core {
namespace features {
namespace impl {

Status CopyPrimFeatureImpl::initialize(PrimitiveFeatureContext *ctx,
                                       const PrimitiveFeature &f) {
  if (f.kind != PrimitiveFeatureKind::Copy) {
    return Status::InvalidParameter() << f.name << ": type was not Copy";
  }

  featureIdx = static_cast<u32>(f.index);

  if (f.references.size() != 1) {
    return Status::InvalidParameter() << f.name
                                      << ": number of parameters must be 1";
  }

  JPP_RETURN_IF_ERROR(ctx->checkFieldType(
      fieldIdx, {spec::ColumnType::String, spec::ColumnType::Int}));

  fieldIdx = static_cast<u32>(f.references.at(0));

  return Status::Ok();
}

Status ProvidedPrimFeatureImpl::initialize(PrimitiveFeatureContext *ctx,
                                           const PrimitiveFeature &f) {
  if (f.kind != PrimitiveFeatureKind::Provided) {
    return Status::InvalidParameter() << f.name << ": type was not Provided";
  }

  featureIdx = static_cast<u32>(f.index);
  if (f.references.size() != 1) {
    return Status::InvalidParameter() << f.name
                                      << ": number of parameters must be 1";
  }

  providedIdx = static_cast<u32>(f.references.at(0));

  JPP_RETURN_IF_ERROR(ctx->checkProvidedFeature(providedIdx));

  return Status::Ok();
}

Status MatchDicPrimFeatureImpl::initialize(PrimitiveFeatureContext *ctx,
                                           const PrimitiveFeature &f) {
  if (f.kind != PrimitiveFeatureKind::MatchDic) {
    return Status::InvalidParameter() << f.name << ": type was not MatchDic";
  }

  featureIdx = static_cast<u32>(f.index);

  if (f.references.size() != 1) {
    return Status::InvalidParameter() << f.name
                                      << ": number of parameters must be 1";
  }

  fieldIdx = static_cast<u32>(f.references.at(0));

  JPP_RETURN_IF_ERROR(ctx->checkFieldType(
      fieldIdx, {spec::ColumnType::String, spec::ColumnType::Int}));

  matchData = f.matchData;

  return Status::Ok();
}

Status MatchAnyDicPrimFeatureImpl::initialize(PrimitiveFeatureContext *ctx,
                                              const PrimitiveFeature &f) {
  if (f.kind != PrimitiveFeatureKind::MatchAnyDic) {
    return Status::InvalidParameter() << f.name << ": type was not MatchAnyDic";
  }

  featureIdx = static_cast<u32>(f.index);

  if (f.references.size() != 1) {
    return Status::InvalidParameter() << f.name
                                      << ": number of parameters must be 1";
  }

  fieldIdx = static_cast<u32>(f.references.at(0));

  JPP_RETURN_IF_ERROR(
      ctx->checkFieldType(fieldIdx, {spec::ColumnType::StringList}));

  matchData = f.matchData;

  return Status::Ok();
}

Status PrimitiveFeatureContext::checkFieldType(
    i32 field, std::initializer_list<spec::ColumnType> columnTypes) const {
  auto &fld = fields->at(field);

  for (auto tp : columnTypes) {
    if (tp == fld.columnType) {
      return Status::Ok();
    }
  }

  auto status = Status::InvalidParameter();
  status << "dic field " << fld.name << " had type " << fld.columnType
         << " , allowed=[";
  for (auto tp : columnTypes) {
    status << tp << " ";
  }

  status << "]";

  return status;
}

Status PrimitiveFeaturesDynamicHolder::initialize(
    PrimitiveFeatureContext *ctx,
    util::ArraySlice<PrimitiveFeature> featureData) {
  features_.reserve(featureData.size());
  for (auto &f : featureData) {
    std::unique_ptr<PrimitiveFeatureImpl> feature;
    switch (f.kind) {
      case PrimitiveFeatureKind::Copy:
        feature.reset(new DynamicPrimitiveFeature<CopyPrimFeatureImpl>{});
        break;
      case PrimitiveFeatureKind::Provided:
        feature.reset(new DynamicPrimitiveFeature<ProvidedPrimFeatureImpl>{});
        break;
      case PrimitiveFeatureKind::MatchDic:
        feature.reset(new DynamicPrimitiveFeature<MatchDicPrimFeatureImpl>{});
        break;
      case PrimitiveFeatureKind::MatchAnyDic:
        feature.reset(
            new DynamicPrimitiveFeature<MatchAnyDicPrimFeatureImpl>{});
        break;
    }
    JPP_RETURN_IF_ERROR(feature->initialize(ctx, f));
  }
  return Status::Ok();
}

void PrimitiveFeaturesDynamicHolder::apply(
    PrimitiveFeatureContext *ctx, EntryPtr entryPtr,
    const util::ArraySlice<i32> &entry,
    util::MutableArraySlice<u64> *features) const noexcept {
  for (auto &f : features_) {
    f->apply(ctx, entryPtr, entry, features);
  }
}
}  // impl
}  // features
}  // core
}  // jumanpp