//
// Created by Arseny Tolmachev on 2017/03/03.
//

#ifndef JUMANPP_FEATURE_API_H
#define JUMANPP_FEATURE_API_H

#include <memory>
#include "runtime_info.h"

namespace jumanpp {
namespace core {

class CoreHolder;

namespace features {

namespace impl {
class PrimitiveFeatureContext;
class ComputeFeatureContext;
class PrimitiveFeatureData;
class PatternFeatureData;
class NgramFeatureData;
}  // namespace impl

class FeatureApply {
 public:
  virtual ~FeatureApply() = default;
};

class PrimitiveFeatureApply : public FeatureApply {
 public:
  virtual void applyBatch(impl::PrimitiveFeatureContext* ctx,
                          impl::PrimitiveFeatureData* data) const noexcept = 0;
};

class ComputeFeatureApply : public FeatureApply {
 public:
  virtual void applyBatch(impl::ComputeFeatureContext* ctx,
                          impl::PrimitiveFeatureData* data) const noexcept = 0;
};

class PatternFeatureApply : public FeatureApply {
 public:
  virtual void applyBatch(impl::PatternFeatureData* data) const noexcept = 0;
};

class NgramFeatureApply : public FeatureApply {
 public:
  virtual void applyBatch(impl::NgramFeatureData* data) const noexcept = 0;
};

class StaticFeatureFactory : public FeatureApply {
 public:
  virtual u64 runtimeHash() const = 0;
  virtual PrimitiveFeatureApply* primitive() const = 0;
  virtual ComputeFeatureApply* compute() const = 0;
  virtual PatternFeatureApply* pattern() const = 0;
  virtual NgramFeatureApply* ngram() const = 0;
};

struct FeatureHolder {
  std::unique_ptr<features::PrimitiveFeatureApply> primitiveDynamic;
  std::unique_ptr<features::PrimitiveFeatureApply> primitiveStatic;
  features::PrimitiveFeatureApply* primitive = nullptr;
  std::unique_ptr<features::ComputeFeatureApply> computeDynamic;
  std::unique_ptr<features::ComputeFeatureApply> computeStatic;
  features::ComputeFeatureApply* compute = nullptr;
  std::unique_ptr<features::PatternFeatureApply> patternDynamic;
  std::unique_ptr<features::PatternFeatureApply> patternStatic;
  features::PatternFeatureApply* pattern = nullptr;
  std::unique_ptr<features::NgramFeatureApply> ngramDynamic;
  std::unique_ptr<features::NgramFeatureApply> ngramStatic;
  features::NgramFeatureApply* ngram = nullptr;

  Status validate() const;
};

Status makeFeatures(const CoreHolder& core, const StaticFeatureFactory* sff,
                    FeatureHolder* result);

}  // namespace features
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_FEATURE_API_H
