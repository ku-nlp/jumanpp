//
// Created by Arseny Tolmachev on 2017/03/03.
//

#ifndef JUMANPP_FEATURE_API_H
#define JUMANPP_FEATURE_API_H

#include <memory>
#include "core/core_types.h"
#include "util/memory.hpp"
#include "util/sliceable_array.h"
#include "util/status.hpp"

namespace jumanpp {
namespace core {

class CoreHolder;

namespace analysis {
class FeatureScorer;
}

namespace dic {
class DictionaryEntries;
}

namespace features {

namespace impl {
class PrimitiveFeatureContext;
class ComputeFeatureContext;
class PrimitiveFeatureData;
class PatternFeatureData;
class NgramFeatureData;
class PatternDynamicApplyImpl;
class NgramDynamicFeatureApply;
class PartialNgramDynamicFeatureApply;
}  // namespace impl

struct FeatureBuffer {
  u32 currentElems;
  util::MutableArraySlice<u64> t1Buffer;
  util::MutableArraySlice<u64> t2Buffer1;
  util::MutableArraySlice<u64> t2Buffer2;
  util::MutableArraySlice<u32> valueBuffer1;
  util::MutableArraySlice<u32> valueBuffer2;
  util::MutableArraySlice<float> scoreBuffer;

  util::MutableArraySlice<u32> valBuf1(u32 size) const {
    return util::MutableArraySlice<u32>{valueBuffer1, 0, size};
  }

  util::MutableArraySlice<u32> valBuf2(u32 size) const {
    return util::MutableArraySlice<u32>{valueBuffer2, 0, size};
  }

  util::Sliceable<u64> t1Buf(u32 numBigrams, u32 numElems) const {
    util::MutableArraySlice<u64> slice{t1Buffer, 0, numBigrams * numElems};
    return util::Sliceable<u64>{slice, numBigrams, numElems};
  }

  util::Sliceable<u64> t2Buf1(u32 numTrigrams, u32 numElems) const {
    util::MutableArraySlice<u64> slice{t2Buffer1, 0, numTrigrams * numElems};
    return util::Sliceable<u64>{slice, numTrigrams, numElems};
  }

  util::Sliceable<u64> t2Buf2(u32 numTrigrams, u32 numElems) const {
    util::MutableArraySlice<u64> slice{t2Buffer2, 0, numTrigrams * numElems};
    return util::Sliceable<u64>{slice, numTrigrams, numElems};
  }

  util::MutableArraySlice<float> scoreBuf(u32 size) const {
    return util::MutableArraySlice<float>{scoreBuffer, 0, size};
  }
};

class FeatureApply {
 public:
  virtual ~FeatureApply() noexcept = default;
};

class PatternFeatureApply : public FeatureApply {
 public:
  virtual void applyBatch(impl::PrimitiveFeatureContext* pfc,
                          impl::PrimitiveFeatureData* data) const noexcept = 0;
};

class GeneratedPatternFeatureApply : public FeatureApply {
 public:
  virtual void patternsAndUnigramsApply(
      ::jumanpp::core::features::impl::PrimitiveFeatureContext* ctx,
      ::jumanpp::util::ArraySlice<::jumanpp::core::NodeInfo> nodeInfos,
      FeatureBuffer* fbuffer,
      ::jumanpp::util::Sliceable<::jumanpp::u64> patternMatrix,
      const ::jumanpp::core::analysis::FeatureScorer* scorer,
      ::jumanpp::util::MutableArraySlice<float> scores) const = 0;
};

class NgramFeatureApply : public FeatureApply {
 public:
  virtual void applyBatch(impl::NgramFeatureData* data) const noexcept = 0;
};

struct AnalysisRunStats {
  u32 maxStarts;
  u32 maxEnds;
};

class PartialNgramFeatureApply : public FeatureApply {
 public:
  virtual void allocateBuffers(FeatureBuffer* buffer,
                               const AnalysisRunStats& stats,
                               util::memory::PoolAlloc* alloc) const = 0;

  virtual void applyUni(FeatureBuffer* buffers, util::ConstSliceable<u64> p0,
                        analysis::FeatureScorer* scorer,
                        util::MutableArraySlice<float> result) const
      noexcept = 0;
  virtual void applyBiStep1(FeatureBuffer* buffers,
                            util::ConstSliceable<u64> p0) const noexcept = 0;
  virtual void applyBiStep2(FeatureBuffer* buffers, util::ArraySlice<u64> p1,
                            analysis::FeatureScorer* scorer,
                            util::MutableArraySlice<float> result) const
      noexcept = 0;
  virtual void applyTriStep1(FeatureBuffer* buffers,
                             util::ConstSliceable<u64> p0) const noexcept = 0;
  virtual void applyTriStep2(FeatureBuffer* buffers,
                             util::ArraySlice<u64> p1) const noexcept = 0;
  virtual void applyTriStep3(FeatureBuffer* buffers, util::ArraySlice<u64> p2,
                             analysis::FeatureScorer* scorer,
                             util::MutableArraySlice<float> result) const
      noexcept = 0;

  virtual void applyBiTri(FeatureBuffer* buffers, util::ArraySlice<u64> t0,
                          util::ConstSliceable<u64> t1,
                          util::ConstSliceable<u64> t2,
                          util::ArraySlice<u32> t1idxes,
                          analysis::FeatureScorer* scorer,
                          util::MutableArraySlice<float> result) const
      noexcept = 0;
};

class StaticFeatureFactory : public FeatureApply {
 public:
  virtual u64 runtimeHash() const { return 0; }
  virtual GeneratedPatternFeatureApply* pattern() const { return nullptr; }
  virtual NgramFeatureApply* ngram() const { return nullptr; }
  virtual PartialNgramFeatureApply* ngramPartial() const { return nullptr; }
};

struct FeatureHolder {
  std::unique_ptr<features::PatternFeatureApply> patternDynamic;
  std::unique_ptr<features::GeneratedPatternFeatureApply> patternStatic;
  std::unique_ptr<features::impl::NgramDynamicFeatureApply> ngramDynamic;
  std::unique_ptr<features::NgramFeatureApply> ngramStatic;
  features::NgramFeatureApply* ngram = nullptr;
  std::unique_ptr<impl::PartialNgramDynamicFeatureApply> ngramPartialDynamic;
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
