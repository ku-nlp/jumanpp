//
// Created by Arseny Tolmachev on 2017/03/03.
//

#include "feature_impl_compute.h"

namespace jumanpp {
namespace core {
namespace features {
namespace impl {

Hasher ExprComputeFeatureImpl::mixPrmitives(
    Hasher input, PrimitiveFeatureContext *ctx, const NodeInfo &nodeInfo,
    const util::ArraySlice<i32> &entry) const {
  auto check = condition_->access(ctx, nodeInfo, entry);
  if (check != 0) {
    for (auto f : true_) {
      input = input.mix(f->access(ctx, nodeInfo, entry));
    }
  } else {
    for (auto f : false_) {
      input = input.mix(f->access(ctx, nodeInfo, entry));
    }
  }
  return input;
}

Status ExprComputeFeatureImpl::initialize(
    const spec::ComputationFeatureDescriptor &fd,
    const std::vector<std::unique_ptr<PrimitiveFeatureImpl>> &primitives,
    const std::vector<std::unique_ptr<ComputeFeatureImpl>> &compute) {
  auto check = [&](i32 idx) -> Status {
    if (idx >= primitives.size()) {
      return JPPS_INVALID_PARAMETER << "a primitive feature idx=" << idx
                                    << "was not available";
    }
    return Status::Ok();
  };

  featureNum_ = static_cast<u32>(fd.index);
  JPP_RETURN_IF_ERROR(check(fd.primitiveFeature));
  this->condition_ = primitives[fd.primitiveFeature].get();

  for (auto &f : fd.trueBranch) {
    JPP_RETURN_IF_ERROR(check(f));
    true_.push_back(primitives[f].get());
  }

  for (auto &f : fd.falseBranch) {
    JPP_RETURN_IF_ERROR(check(f));
    false_.push_back(primitives[f].get());
  }

  return Status::Ok();
}

ExprComputeFeatureImpl::~ExprComputeFeatureImpl() = default;

Hasher NoopComputeFeatureImpl::mixPrmitives(
    Hasher input, PrimitiveFeatureContext *ctx, const NodeInfo &nodeInfo,
    const util::ArraySlice<i32> &entry) const {
  return input.mix(prim_->access(ctx, nodeInfo, entry));
}

Status NoopComputeFeatureImpl::initialize(
    const spec::ComputationFeatureDescriptor &fd,
    const std::vector<std::unique_ptr<PrimitiveFeatureImpl>> &primitives,
    const std::vector<std::unique_ptr<ComputeFeatureImpl>> &compute) {
  if (fd.primitiveFeature >= primitives.size()) {
    return JPPS_INVALID_PARAMETER
           << "primitive feature idx=" << fd.primitiveFeature
           << " was not available";
  }
  featureNum_ = static_cast<u32>(fd.index);
  prim_ = primitives[fd.primitiveFeature].get();
  return Status::Ok();
}

NoopComputeFeatureImpl::~NoopComputeFeatureImpl() = default;
}  // namespace impl
}  // namespace features
}  // namespace core
}  // namespace jumanpp