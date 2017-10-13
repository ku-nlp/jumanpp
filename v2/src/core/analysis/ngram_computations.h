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
class AnalyzerImpl;
class FeatureScorer;

class NgramScoreHolder {
 public:
  void prepare(const NGramFeatureComputer& computer);
  void newBoundary(i32 numLeft, i32 numRight);
};

class NGramFeatureComputer {
  u32 num1Grams_ = 0;
  u32 num2Grams_ = 0;
  u32 num3Grams_ = 0;
  u32 maxStarts_ = 0;
  u32 currentSize_ = 0;

  const AnalyzerImpl* analyzer_;
  const FeatureScorer* scorer_;
  const features::PartialNgramFeatureApply* ngramApply_;
  util::MutableArraySlice<u64> buffer1_;
  util::MutableArraySlice<u64> buffer2_;
  util::MutableArraySlice<u32> featureBuffer_;
  util::ConstSliceable<u32> featureSubset_;

 public:
  NGramFeatureComputer(const AnalyzerImpl* analyzer,
                       const FeatureScorer* scorer);

  Status initialize();

  void compute1Grams(i32 boundary);
  void compute2GramsStep1(i32 boundary);
  void compute2GramsStep2(i32 boundary, i32 position);
  void compute3GramsStep1(i32 boundary);
  void compute3GramsStep2(i32 boundary, i32 position);
  void compute3GramsStep3(i32 boundary, i32 position);
  void addScores(util::MutableArraySlice<float> result);
};

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_NGRAM_COMPUTATIONS_H
