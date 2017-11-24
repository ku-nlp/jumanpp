//
// Created by Arseny Tolmachev on 2017/02/28.
//

#include "feature_impl_prim.h"

namespace jumanpp {
namespace core {
namespace features {
namespace impl {

Status CopyPrimFeatureImpl::initialize(
    FeatureConstructionContext *ctx,
    const spec::PrimitiveFeatureDescriptor &f) {
  if (f.kind != PrimitiveFeatureKind::Copy) {
    return Status::InvalidParameter() << f.name << ": type was not Copy";
  }

  if (f.references.size() != 1) {
    return Status::InvalidParameter()
           << f.name << ": number of parameters must be 1";
  }

  fieldIdx = static_cast<u32>(f.references.at(0));

  JPP_RETURN_IF_ERROR(ctx->checkFieldType(
      fieldIdx, {spec::FieldType::String, spec::FieldType::Int}));

  return Status::Ok();
}

Status ProvidedPrimFeatureImpl::initialize(
    FeatureConstructionContext *ctx,
    const spec::PrimitiveFeatureDescriptor &f) {
  if (f.kind != PrimitiveFeatureKind::Provided) {
    return Status::InvalidParameter() << f.name << ": type was not Provided";
  }

  if (f.references.size() != 1) {
    return Status::InvalidParameter()
           << f.name << ": number of parameters must be 1";
  }

  providedIdx = static_cast<u32>(f.references.at(0));

  JPP_RETURN_IF_ERROR(ctx->checkProvidedFeature(providedIdx));

  return Status::Ok();
}

Status ByteLengthPrimFeatureImpl::initialize(
    FeatureConstructionContext *ctx,
    const spec::PrimitiveFeatureDescriptor &f) {
  if (f.kind != PrimitiveFeatureKind::ByteLength) {
    return Status::InvalidParameter() << f.name << ": type was not Length";
  }

  if (f.references.size() != 1) {
    return Status::InvalidParameter()
           << f.name << ": number of parameters must be 1";
  }

  fieldIdx = static_cast<u32>(f.references.at(0));

  JPP_RETURN_IF_ERROR(ctx->setLengthField(fieldIdx, &field));

  return Status::Ok();
}

Status CodepointLengthPrimFeatureImpl::initialize(
    FeatureConstructionContext *ctx,
    const spec::PrimitiveFeatureDescriptor &f) {
  if (f.kind != PrimitiveFeatureKind::CodepointSize) {
    return Status::InvalidParameter()
           << f.name << ": type was not CodepointSize";
  }

  if (f.references.size() != 1) {
    return Status::InvalidParameter()
           << f.name << ": number of parameters must be 1";
  }

  fieldIdx = static_cast<u32>(f.references.at(0));

  JPP_RETURN_IF_ERROR(ctx->setLengthField(fieldIdx, &field));

  return Status::Ok();
}

Status FeatureConstructionContext::checkFieldType(
    i32 field, std::initializer_list<spec::FieldType> columnTypes) const {
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

Status CodepointFeatureImpl::initialize(
    FeatureConstructionContext *ctx,
    const spec::PrimitiveFeatureDescriptor &f) {
  if (f.references.size() != 1) {
    return JPPS_INVALID_PARAMETER << "match data of feature [" << f.name
                                  << "] should be 1, was "
                                  << f.references.size();
  }
  this->offset = f.references[0];
  return Status::Ok();
}

Status CodepointTypeFeatureImpl::initialize(
    FeatureConstructionContext *ctx,
    const spec::PrimitiveFeatureDescriptor &f) {
  if (f.references.size() != 1) {
    return JPPS_INVALID_PARAMETER << "match data of feature [" << f.name
                                  << "] should be 1, was "
                                  << f.references.size();
  }
  this->offset = f.references[0];
  return Status::Ok();
}

Status SurfaceCodepointLengthPrimFeatureImpl::initialize(
    FeatureConstructionContext *ctx,
    const spec::PrimitiveFeatureDescriptor &f) {
  if (!f.references.empty()) {
    return JPPS_INVALID_PARAMETER << "reference size should be empty";
  }
  return Status::Ok();
}

Status ShiftMaskPrimFeatureImpl::initialize(
    FeatureConstructionContext *ctx,
    const spec::PrimitiveFeatureDescriptor &f) {
  if (f.references.size() != 2) {
    return JPPS_INVALID_PARAMETER << "expected reference size to be 2, was: "
                                  << f.references.size();
  }
  fieldIdx_ = static_cast<u32>(f.references[0]);
  shift_ = static_cast<u32>(f.references[1]);
  mask_ = 0x1;
  return Status::Ok();
}
}  // namespace impl
}  // namespace features
}  // namespace core
}  // namespace jumanpp