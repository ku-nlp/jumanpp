//
// Created by Arseny Tolmachev on 2017/10/12.
//

#ifndef JUMANPP_NGRAM_COMPUTATIONS_H
#define JUMANPP_NGRAM_COMPUTATIONS_H

#include <util/sliceable_array.h>
#include "core/features_api.h"
#include "util/array_slice.h"
#include "util/types.hpp"

namespace jumanpp {
namespace core {
namespace analysis {

class Lattice;
class AnalyzerImpl;
class FeatureScorer;

class NgramScoreHolder {
  u32 curSize_;
  util::MutableArraySlice<float> bufferT0_;
  util::MutableArraySlice<float> bufferT1_;
  util::MutableArraySlice<float> bufferT2_;

 public:
  util::MutableArraySlice<float> bufferT0() {
    return util::MutableArraySlice<float>{bufferT0_, 0, curSize_};
  }

  util::MutableArraySlice<float> bufferT1() {
    return util::MutableArraySlice<float>{bufferT1_, 0, curSize_};
  }

  util::MutableArraySlice<float> bufferT2() {
    return util::MutableArraySlice<float>{bufferT2_, 0, curSize_};
  }

  void prepare(util::memory::ManagedAllocatorCore* alloc, u32 maxNodes);

  void newBoundary(u32 numRight) { curSize_ = numRight; }
};

struct NgramStats {
  u32 num1Grams = 0;
  u32 num2Grams = 0;
  u32 num3Grams = 0;
  u32 maxNGrams = 0;

  void initialze(const features::FeatureRuntimeInfo* info);
};

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_NGRAM_COMPUTATIONS_H
