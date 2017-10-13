//
// Created by Arseny Tolmachev on 2017/03/07.
//

#ifndef JUMANPP_SCORE_PROCESSOR_H
#define JUMANPP_SCORE_PROCESSOR_H

#include "core/analysis/lattice_config.h"
#include "core/analysis/ngram_computations.h"
#include "core/analysis/score_api.h"
#include "core/features_api.h"
#include "util/memory.hpp"

namespace jumanpp {
namespace core {
namespace analysis {

class Lattice;
class LatticeBoundary;
class LatticeBoundaryConnection;
class AnalyzerImpl;

// this class is zero-weight heap on top of other storage
class EntryBeam {
  util::MutableArraySlice<ConnectionBeamElement> elements_;

 public:
  EntryBeam(const util::MutableArraySlice<ConnectionBeamElement>& elements_)
      : elements_(elements_) {}

  static bool compare(const ConnectionBeamElement& left,
                      const ConnectionBeamElement& right) {
    return left.totalScore > right.totalScore;
  }

  static void initializeBlock(
      util::MutableArraySlice<ConnectionBeamElement> elems) {
    u16 pat = (u16)(0xffffu);
    auto val = std::numeric_limits<Score>::lowest();
    ConnectionBeamElement zero{{pat, pat, pat, pat}, val};
    std::fill(elems.begin(), elems.end(), zero);
  }

  void pushItem(const ConnectionBeamElement& item) {
    // keep min-heap (minimum element at 0)
    // if our element is better than minimum, swap them
    auto& tip = elements_[0];
    if (compare(item, tip)) {
      std::pop_heap(elements_.begin(), elements_.end(), compare);
      elements_.back() = item;
      std::push_heap(elements_.begin(), elements_.end(), compare);
    }
  }

  static void fixupBeam(util::MutableArraySlice<ConnectionBeamElement>& data) {
    std::sort_heap(data.begin(), data.end(), compare);
  }

  static bool isFake(const ConnectionBeamElement& e) {
    u16 pat = (u16)(0xffffu);
    return e.ptr.boundary == pat && e.ptr.left == pat && e.ptr.right == pat;
  }
};

class ScoreProcessor {
  Lattice* lattice_;
  NGramFeatureComputer ngram_;
  util::ArraySlice<ConnectionBeamElement> beamPtrs_;
  NgramScoreHolder scores_;
  i32 beamSize_ = 0;

  explicit ScoreProcessor(AnalyzerImpl* analyzer);

 public:
  static std::pair<Status, ScoreProcessor*> make(AnalyzerImpl* impl);

  i32 activeBeamSize() const { return beamSize_; }

  void resolveBeamAt(i32 boundary, i32 position);
  void startBoundary(u32 currentNodes);
  void applyT0(i32 boundary, FeatureScorer* features);
  void applyT1(i32 boundary, i32 position, FeatureScorer* features);
  void applyT2(i32 beamIdx, FeatureScorer* features);
  void copyFeatureScores(i32 beam, LatticeBoundaryConnection* bndconn);

  void updateBeams(i32 boundary, i32 endPos, LatticeBoundary* bnd,
                   LatticeBoundaryConnection* bndconn, const ScorerDef* sc);
};

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_SCORE_PROCESSOR_H
