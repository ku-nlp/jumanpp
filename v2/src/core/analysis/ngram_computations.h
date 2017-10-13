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

class NGramFeatureComputer;
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

  void prepare(AnalyzerImpl* analyzer, u32 maxNodes);

  void newBoundary(u32 numRight) { curSize_ = numRight; }
};

struct NgramStats {
  u32 num1Grams = 0;
  u32 num2Grams = 0;
  u32 num3Grams = 0;
  u32 maxNGrams = 0;

  void initialze(const features::FeatureRuntimeInfo* info);
};

class NGramFeatureComputer {
  u32 maxStarts_ = 0;
  u32 currentSize_ = 0;

  const NgramStats& stats_;
  const Lattice* lattice_;
  const features::PartialNgramFeatureApply* ngramApply_;
  util::MutableArraySlice<u64> biBuffer_;
  util::MutableArraySlice<u64> triBuffer1_;
  util::MutableArraySlice<u64> triBuffer2_;
  util::MutableArraySlice<u32> featureBuffer_;
  util::ConstSliceable<u32> featureSubset_;

 public:
  explicit NGramFeatureComputer(const AnalyzerImpl* analyzer);

  Status initialize(const AnalyzerImpl* analyzer);

  void computeUnigrams(i32 boundary);
  void computeBigramsStep1(i32 boundary);
  void computeBigramsStep2(i32 boundary, i32 position);
  void computeTrigramsStep1(i32 boundary);
  void computeTrigramsStep2(i32 boundary, i32 position);
  void compute3GramsStep3(i32 boundary, i32 position);
  void setScores(FeatureScorer* scorer, util::MutableArraySlice<float> result);
  void addScores(FeatureScorer* scorer, util::ArraySlice<float> source,
                 util::MutableArraySlice<float> result);
  u32 maxStarts() const { return maxStarts_; }
};

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_NGRAM_COMPUTATIONS_H
