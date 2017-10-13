//
// Created by Arseny Tolmachev on 2017/10/12.
//

#include "ngram_computations.h"
#include "core/analysis/analyzer_impl.h"

namespace jumanpp {
namespace core {
namespace analysis {

void NGramFeatureComputer::computeUnigrams(i32 boundary) {
  auto l = lattice_;
  auto b = l->boundary(boundary);
  auto patterns = b->starts()->patternFeatureData();
  auto rows = patterns.numRows();
  auto cols = stats_.num1Grams;
  util::MutableArraySlice<u32> buffer{featureBuffer_, 0, rows * cols};
  util::Sliceable<u32> result{buffer, cols, rows};
  ngramApply_->applyUni(patterns, result);
  featureSubset_ = result;
}

void NGramFeatureComputer::computeBigramsStep1(i32 boundary) {
  auto l = lattice_;
  auto b = l->boundary(boundary);
  auto patterns = b->starts()->patternFeatureData();
  auto rows = patterns.numRows();
  auto cols = stats_.num2Grams;
  util::MutableArraySlice<u64> buffer{biBuffer_, 0, rows * cols};
  util::Sliceable<u64> result{buffer, cols, rows};
  ngramApply_->applyBiStep1(patterns, result);
  currentSize_ = static_cast<u32>(rows);
}

void NGramFeatureComputer::computeBigramsStep2(i32 boundary, i32 position) {
  auto l = lattice_;
  auto b = l->boundary(boundary);
  const auto patterns = b->starts()->patternFeatureData();
  auto rows = currentSize_;
  auto cols = stats_.num2Grams;
  auto newItem = patterns.row(position);
  util::ArraySlice<u64> stateData{biBuffer_, 0, rows * cols};
  util::ConstSliceable<u64> state{stateData, cols, rows};
  util::MutableArraySlice<u32> resultData{featureBuffer_, 0, rows * cols};
  util::Sliceable<u32> result{resultData, cols, rows};
  ngramApply_->applyBiStep2(state, newItem, result);
  featureSubset_ = result;
}

void NGramFeatureComputer::computeTrigramsStep1(i32 boundary) {
  auto l = lattice_;
  auto b = l->boundary(boundary);
  auto patterns = b->starts()->patternFeatureData();
  auto rows = patterns.numRows();
  auto cols = stats_.num3Grams;
  util::MutableArraySlice<u64> buffer{triBuffer1_, 0, rows * cols};
  util::Sliceable<u64> result{buffer, cols, rows};
  ngramApply_->applyTriStep1(patterns, result);
  currentSize_ = static_cast<u32>(rows);
}

void NGramFeatureComputer::computeTrigramsStep2(i32 boundary, i32 position) {
  auto l = lattice_;
  auto b = l->boundary(boundary);
  const auto patterns = b->starts()->patternFeatureData();
  auto rows = currentSize_;
  auto cols = stats_.num3Grams;
  auto newItem = patterns.row(position);
  util::ArraySlice<u64> stateData{triBuffer1_, 0, rows * cols};
  util::ConstSliceable<u64> state{stateData, cols, rows};
  util::MutableArraySlice<u64> resultData{triBuffer2_, 0, rows * cols};
  util::Sliceable<u64> result{resultData, cols, rows};
  ngramApply_->applyTriStep2(state, newItem, result);
}

void NGramFeatureComputer::compute3GramsStep3(i32 boundary, i32 position) {
  auto l = lattice_;
  auto b = l->boundary(boundary);
  const auto patterns = b->starts()->patternFeatureData();
  auto rows = currentSize_;
  auto cols = stats_.num3Grams;
  auto newItem = patterns.row(position);
  util::ArraySlice<u64> stateData{triBuffer2_, 0, rows * cols};
  util::ConstSliceable<u64> state{stateData, cols, rows};
  util::MutableArraySlice<u32> resultData{featureBuffer_, 0, rows * cols};
  util::Sliceable<u32> result{resultData, cols, rows};
  ngramApply_->applyTriStep3(state, newItem, result);
  featureSubset_ = result;
}

void NGramFeatureComputer::setScores(FeatureScorer *scorer,
                                     util::MutableArraySlice<float> result) {
  scorer->compute(result, featureSubset_);
}

void NGramFeatureComputer::addScores(FeatureScorer *scorer,
                                     util::ArraySlice<float> source,
                                     util::MutableArraySlice<float> result) {
  scorer->add(source, result, featureSubset_);
}

Status NGramFeatureComputer::initialize(const AnalyzerImpl *analyzer) {
  auto l = lattice_;
  for (int i = 0; i < l->createdBoundaryCount(); ++i) {
    auto b = l->boundary(i);
    auto curStarts = static_cast<const u32 &>(b->starts()->arraySize());
    maxStarts_ = std::max(curStarts, maxStarts_);
  }

  auto alloc = analyzer->alloc();
  auto elemCnt = stats_.maxNGrams * maxStarts_;

  biBuffer_ = alloc->allocateBuf<u64>(elemCnt);
  triBuffer1_ = alloc->allocateBuf<u64>(elemCnt);
  triBuffer2_ = alloc->allocateBuf<u64>(elemCnt);
  featureBuffer_ = alloc->allocateBuf<u32>(elemCnt);

  return Status::Ok();
}

NGramFeatureComputer::NGramFeatureComputer(const AnalyzerImpl *analyzer)
    : stats_{analyzer->ngramStats()},
      lattice_{analyzer->lattice()},
      ngramApply_{analyzer->core().features().ngramPartial} {}

void NgramStats::initialze(const features::FeatureRuntimeInfo *info) {
  num1Grams = 0;
  num2Grams = 0;
  num3Grams = 0;
  auto &ngramInfo = info->ngrams;
  for (auto &ng : ngramInfo) {
    auto order = ng.arguments.size();
    if (order == 1) {
      num1Grams += 1;
    } else if (order == 2) {
      num2Grams += 1;
    } else if (order == 3) {
      num3Grams += 1;
    }
  }
  maxNGrams = std::max({num1Grams, num2Grams, num3Grams});
}

void NgramScoreHolder::prepare(AnalyzerImpl *analyzer, u32 maxNodes) {
  auto alloc = analyzer->alloc();
  bufferT0_ = alloc->allocateBuf<float>(maxNodes);
  bufferT1_ = alloc->allocateBuf<float>(maxNodes);
  bufferT2_ = alloc->allocateBuf<float>(maxNodes);
}

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp