//
// Created by Arseny Tolmachev on 2017/03/03.
//

#ifndef JUMANPP_FEATURE_IMPL_COMPUTE_H
#define JUMANPP_FEATURE_IMPL_COMPUTE_H

#include "core/features_api.h"
#include "core/impl/feature_impl_prim.h"
#include "core/impl/feature_impl_types.h"
#include "core/impl/feature_types.h"
#include "util/fast_hash.h"

namespace jumanpp {
namespace core {
namespace features {
namespace impl {

using Hash = util::hashing::FastHash1;

class ComputeFeatureImpl {
 public:
  virtual ~ComputeFeatureImpl() = default;
  virtual Hash mixPrmitives(Hash input, PrimitiveFeatureContext* ctx,
                            const NodeInfo& nodeInfo,
                            const util::ArraySlice<i32>& entry) const = 0;

  virtual Status initialize(
      const spec::ComputationFeatureDescriptor& fd,
      const std::vector<std::unique_ptr<PrimitiveFeatureImpl>>& primitives,
      const std::vector<std::unique_ptr<ComputeFeatureImpl>>& compute) = 0;
};

class NoopComputeFeatureImpl : public ComputeFeatureImpl {
  u32 featureNum_;
  const PrimitiveFeatureImpl* prim_;

 public:
  virtual Hash mixPrmitives(Hash input, PrimitiveFeatureContext* ctx,
                            const NodeInfo& nodeInfo,
                            const util::ArraySlice<i32>& entry) const override;
  virtual ~NoopComputeFeatureImpl() override;
  Status initialize(
      const spec::ComputationFeatureDescriptor& fd,
      const std::vector<std::unique_ptr<PrimitiveFeatureImpl>>& primitives,
      const std::vector<std::unique_ptr<ComputeFeatureImpl>>& compute) override;
};

class ExprComputeFeatureImpl : public ComputeFeatureImpl {
  u32 featureNum_;
  const PrimitiveFeatureImpl* condition_;
  std::vector<const PrimitiveFeatureImpl*> true_;
  std::vector<const PrimitiveFeatureImpl*> false_;

 public:
  virtual Hash mixPrmitives(Hash input, PrimitiveFeatureContext* ctx,
                            const NodeInfo& nodeInfo,
                            const util::ArraySlice<i32>& entry) const override;
  Status initialize(
      const spec::ComputationFeatureDescriptor& fd,
      const std::vector<std::unique_ptr<PrimitiveFeatureImpl>>& primitives,
      const std::vector<std::unique_ptr<ComputeFeatureImpl>>& compute) override;
  virtual ~ExprComputeFeatureImpl() override;
};

}  // namespace impl
}  // namespace features
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_FEATURE_IMPL_COMPUTE_H
