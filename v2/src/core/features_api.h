//
// Created by Arseny Tolmachev on 2017/03/03.
//

#ifndef JUMANPP_FEATURE_API_H
#define JUMANPP_FEATURE_API_H

namespace jumanpp {
namespace core {
namespace features {

namespace impl {
class PrimitiveFeatureContext;
class ComputeFeatureContext;
class PrimitiveFeatureData;
}

class FeatureApply {
 public:
  virtual ~FeatureApply() = 0;
};

class PrimitiveFeatureApply : public FeatureApply {
 public:
  virtual void applyBatch(impl::PrimitiveFeatureContext* ctx,
                          impl::PrimitiveFeatureData* data) const noexcept = 0;
};

}  // features
}  // core
}  // jumanpp

#endif  // JUMANPP_FEATURE_API_H
