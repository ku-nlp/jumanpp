//
// Created by Arseny Tolmachev on 2017/10/12.
//

#include "ngram_computations.h"
#include "core/analysis/analyzer_impl.h"

namespace jumanpp {
namespace core {
namespace analysis {

void NGramFeatureComputer::compute1Grams(i32 boundary) {
  auto l = analyzer_->lattice();
  auto b = l->boundary(boundary);
  auto patterns = b->starts()->patternFeatureData();
  auto rows = patterns.numRows();
  util::MutableArraySlice<u32> buffer{featureBuffer_, 0, rows * num1Grams_};
  util::Sliceable<u32> result{buffer, num1Grams_, rows};
  ngramApply_->applyUni(patterns, result);
  featureSubset_ = result;
}

void NGramFeatureComputer::compute2GramsStep1(i32 boundary) {
  auto l = analyzer_->lattice();
  auto b = l->boundary(boundary);
  auto patterns = b->starts()->patternFeatureData();
  auto rows = patterns.numRows();
  auto cols = num2Grams_;
  util::MutableArraySlice<u64> buffer{buffer1_, 0, rows * cols};
  util::Sliceable<u64> result{buffer, cols, rows};
  ngramApply_->applyBiStep1(patterns, result);
  currentSize_ = static_cast<u32>(rows);
}

void NGramFeatureComputer::compute2GramsStep2(i32 boundary, i32 position) {
  auto l = analyzer_->lattice();
  auto b = l->boundary(boundary);
  const auto patterns = b->starts()->patternFeatureData();
  auto rows = currentSize_;
  auto cols = num2Grams_;
  auto newItem = patterns.row(position);
  util::ArraySlice<u64> stateData{buffer1_, 0, rows * cols};
  util::ConstSliceable<u64> state{stateData, cols, rows};
  util::MutableArraySlice<u32> resultData{featureBuffer_, 0, rows * cols};
  util::Sliceable<u32> result{resultData, cols, rows};
  ngramApply_->applyBiStep2(state, newItem, result);
  featureSubset_ = result;
}

void NGramFeatureComputer::compute3GramsStep1(i32 boundary) {
  auto l = analyzer_->lattice();
  auto b = l->boundary(boundary);
  auto patterns = b->starts()->patternFeatureData();
  auto rows = patterns.numRows();
  auto cols = num3Grams_;
  util::MutableArraySlice<u64> buffer{buffer1_, 0, rows * cols};
  util::Sliceable<u64> result{buffer, cols, rows};
  ngramApply_->applyTriStep1(patterns, result);
  currentSize_ = static_cast<u32>(rows);
}

void NGramFeatureComputer::compute3GramsStep2(i32 boundary, i32 position) {
  auto l = analyzer_->lattice();
  auto b = l->boundary(boundary);
  const auto patterns = b->starts()->patternFeatureData();
  auto rows = currentSize_;
  auto cols = num3Grams_;
  auto newItem = patterns.row(position);
  util::ArraySlice<u64> stateData{buffer1_, 0, rows * cols};
  util::ConstSliceable<u64> state{stateData, cols, rows};
  util::MutableArraySlice<u64> resultData{buffer2_, 0, rows * cols};
  util::Sliceable<u64> result{resultData, cols, rows};
  ngramApply_->applyTriStep2(state, newItem, result);
}

void NGramFeatureComputer::compute3GramsStep3(i32 boundary, i32 position) {
  auto l = analyzer_->lattice();
  auto b = l->boundary(boundary);
  const auto patterns = b->starts()->patternFeatureData();
  auto rows = currentSize_;
  auto cols = num3Grams_;
  auto newItem = patterns.row(position);
  util::ArraySlice<u64> stateData{buffer2_, 0, rows * cols};
  util::ConstSliceable<u64> state{stateData, cols, rows};
  util::MutableArraySlice<u32> resultData{featureBuffer_, 0, rows * cols};
  util::Sliceable<u32> result{resultData, cols, rows};
  ngramApply_->applyTriStep3(state, newItem, result);
  featureSubset_ = result;
}

void NGramFeatureComputer::addScores(util::MutableArraySlice<float> result) {
  scorer_->add(result, featureSubset_);
}

Status NGramFeatureComputer::initialize() {
  auto l = analyzer_->lattice();
  for (int i = 0; i < l->createdBoundaryCount(); ++i) {
    auto b = l->boundary(i);
    auto curStarts = static_cast<const u32 &>(b->starts()->arraySize());
    maxStarts_ = std::max(curStarts, maxStarts_);
  }

  auto &ngramInfo = analyzer_->core().runtime().features.ngrams;
  for (auto &ng : ngramInfo) {
    auto order = ng.arguments.size();
    if (order == 1) {
      num1Grams_ += 1;
    } else if (order == 2) {
      num2Grams_ += 1;
    } else if (order == 3) {
      num3Grams_ += 1;
    } else {
      return JPPS_NOT_IMPLEMENTED << "ngram feature #" << ng.index << " had "
                                  << order
                                  << " components, we only support 1, 2 or 3";
    }
  }

  if (ngramApply_ == nullptr) {
    return JPPS_INVALID_STATE << "ngram partial features were not initialized";
  }

  auto alloc = analyzer_->alloc();

  auto maxNgrams = std::max({num1Grams_, num2Grams_, num3Grams_});
  auto elemCnt = maxNgrams * maxStarts_;

  buffer1_ = alloc->allocateBuf<u64>(elemCnt);
  buffer2_ = alloc->allocateBuf<u64>(elemCnt);
  featureBuffer_ = alloc->allocateBuf<u32>(elemCnt);

  return Status::Ok();
}

NGramFeatureComputer::NGramFeatureComputer(const AnalyzerImpl *analyzer,
                                           const FeatureScorer *scorer)
    : analyzer_{analyzer},
      scorer_{scorer},
      ngramApply_{analyzer->core().features().ngramPartial} {}

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp