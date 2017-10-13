//
// Created by Arseny Tolmachev on 2017/03/03.
//

#ifndef JUMANPP_FEATURE_API_H
#define JUMANPP_FEATURE_API_H

#include <memory>
#include "runtime_info.h"
#include "util/sliceable_array.h"

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
class NgramDynamicFeatureApply;
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

class PartialNgramFeatureApply : public FeatureApply {
 public:
  virtual void applyUni(util::ConstSliceable<u64> p0,
                        util::Sliceable<u32> result) const = 0;
  virtual void applyBiStep1(util::ConstSliceable<u64> p0,
                            util::Sliceable<u64> result) const = 0;
  virtual void applyBiStep2(util::ConstSliceable<u64> state,
                            util::ArraySlice<u64> p1,
                            util::Sliceable<u32> result) const = 0;
  virtual void applyTriStep1(util::ConstSliceable<u64> p0,
                             util::Sliceable<u64> result) const = 0;
  virtual void applyTriStep2(util::ConstSliceable<u64> state,
                             util::ArraySlice<u64> p1,
                             util::Sliceable<u64> result) const = 0;
  virtual void applyTriStep3(util::ConstSliceable<u64> state,
                             util::ArraySlice<u64> p2,
                             util::Sliceable<u32> result) const = 0;
};

class StaticFeatureFactory : public FeatureApply {
 public:
  virtual u64 runtimeHash() const { return 0; }
  virtual PrimitiveFeatureApply* primitive() const { return nullptr; }
  virtual ComputeFeatureApply* compute() const { return nullptr; }
  virtual PatternFeatureApply* pattern() const { return nullptr; }
  virtual NgramFeatureApply* ngram() const { return nullptr; }
  virtual PartialNgramFeatureApply* ngramPartial() const { return nullptr; }
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
  std::unique_ptr<features::impl::NgramDynamicFeatureApply> ngramDynamic;
  std::unique_ptr<features::NgramFeatureApply> ngramStatic;
  features::NgramFeatureApply* ngram = nullptr;
  std::unique_ptr<features::PartialNgramFeatureApply> ngramPartialDynamic;
  std::unique_ptr<features::PartialNgramFeatureApply> ngramPartialStatic;
  features::PartialNgramFeatureApply* ngramPartial = nullptr;

  Status validate() const;

  FeatureHolder();
  ~FeatureHolder();
};

Status makeFeatures(const CoreHolder& core, const StaticFeatureFactory* sff,
                    FeatureHolder* result);

}  // namespace features
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_FEATURE_API_H
