//
// Created by Arseny Tolmachev on 2017/03/07.
//

#ifndef JUMANPP_SCORE_PROCESSOR_H
#define JUMANPP_SCORE_PROCESSOR_H

#include <util/flatmap.h>
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
class LatticeBoundaryScores;
class AnalyzerImpl;
struct AnalyzerConfig;

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
    std::fill(elems.begin(), elems.end(), fake());
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

  util::ArraySlice<ConnectionBeamElement> finalize() {
    std::sort_heap(elements_.begin(), elements_.end(), compare);
    for (size_t i = 0; i < elements_.size(); ++i) {
      if (isFake(elements_[i])) {
        return {elements_.data(), i};
      }
    }
    return elements_;
  }

  bool isTopFake() const { return isFake(elements_[0]); }

  static bool isFake(const ConnectionBeamElement& e) {
    u64 el;
    std::memcpy(&el, &e.ptr, sizeof(u64));
    return (~el) == 0;
  }

  static ConnectionBeamElement fake() {
    u16 pat = (u16)(0xffffu);
    auto val = std::numeric_limits<Score>::lowest();
    ConnectionBeamElement zero{{pat, pat, pat, pat}, val};
    return zero;
  }
};

struct BeamCandidate {
  static_assert(sizeof(Score) == sizeof(u32), "");
  u64 data_;

  BeamCandidate() = default;
  BeamCandidate(Score score, u16 left, u16 beam)
      : data_{pack(score, left, beam)} {}

  Score score() const noexcept {
    u32 value = static_cast<u32>(data_ >> 32);
    value = ((value & 0x8000'0000) == 0) ? ~value : value ^ 0x8000'0000;
    float result;
    std::memcpy(&result, &value, sizeof(float));
    return result;
  }

  u16 left() const noexcept { return static_cast<u16>(data_ >> 16); }

  u16 beam() const noexcept { return static_cast<u16>(data_); }

  bool operator<(const BeamCandidate& o) const noexcept {
    return data_ < o.data_;
  }

  bool operator>(const BeamCandidate& o) const noexcept {
    return data_ > o.data_;
  }

  static u64 pack(Score score, u16 left, u16 beam) noexcept {
    u32 value;
    std::memcpy(&value, &score, sizeof(float));
    value = ((value & 0x8000'0000) != 0) ? ~value : value ^ 0x8000'0000;
    return (static_cast<u64>(value) << 32) | (left << 16) | beam;
  }
};

struct ScoreProcessor {
  i32 beamSize_ = 0;
  util::ArraySlice<ConnectionBeamElement> beamPtrs_;
  NgramScoreHolder scores_;
  features::FeatureBuffer featureBuffer_;
  const features::PatternFeatureApply* patternDynamic_;
  const features::GeneratedPatternFeatureApply* patternStatic_;
  const features::PartialNgramFeatureApply* ngramApply_;
  Lattice* lattice_;
  features::AnalysisRunStats runStats_;
  util::MutableArraySlice<BeamCandidate> beamCandidates_;

  const AnalyzerConfig* cfg_;
  i32 globalBeamSize_;
  util::MutableArraySlice<BeamCandidate> globalBeam_;
  util::FlatMap<LatticeNodePtr, u32> t1PtrData_;
  util::MutableArraySlice<u32> t1positions_;
  util::Sliceable<u64> t1patBuf_;
  util::Sliceable<u64> t2patBuf_;
  util::MutableArraySlice<Score> gbeamScoreBuf_;
  util::MutableArraySlice<u32> beamIdxBuffer_;
  util::Sliceable<Score> t0prescores_;
  util::MutableArraySlice<Score> t0cutoffBuffer_;
  util::MutableArraySlice<u32> t0cutoffIdxBuffer_;

  explicit ScoreProcessor(AnalyzerImpl* analyzer);

 public:  // functions below
  static std::pair<Status, ScoreProcessor*> make(AnalyzerImpl* impl);

  i32 activeBeamSize() const { return beamSize_; }

  void resolveBeamAt(i32 boundary, i32 position);
  void startBoundary(u32 currentNodes);
  void applyT0(i32 boundary, FeatureScorer* features);
  void computeT0All(i32 boundary, FeatureScorer* features,
                    features::impl::PrimitiveFeatureContext* pfc);
  void applyT1(i32 boundary, i32 position, FeatureScorer* features);
  void applyT2(i32 beamIdx, FeatureScorer* features);
  void copyFeatureScores(i32 left, i32 beam, LatticeBoundaryScores* bndconn);

  void makeBeams(i32 boundary, LatticeBoundary* bnd, const ScorerDef* sc);

  // GLOBAL BEAM IMPL
  util::ArraySlice<BeamCandidate> makeGlobalBeam(i32 bndIdx, i32 maxElems);
  void computeGbeamScores(i32 bndIdx, util::ArraySlice<BeamCandidate> gbeam,
                          FeatureScorer* features);

  util::ArraySlice<u32> dedupT1(i32 bndIdx,
                                util::ArraySlice<BeamCandidate> gbeam);
  util::Sliceable<u64> gatherT1();
  util::Sliceable<u64> gatherT2(i32 bndIdx,
                                util::ArraySlice<BeamCandidate> gbeam);
  void copyT0Scores(i32 bndIdx, i32 t0idx,
                    util::ArraySlice<BeamCandidate> gbeam,
                    util::MutableArraySlice<Score> scores, Score t0Score);
  void makeT0Beam(i32 bndIdx, i32 t0idx, util::ArraySlice<BeamCandidate> gbeam,
                  util::MutableArraySlice<Score> scores);

  void computeT0Prescores(util::ArraySlice<BeamCandidate> gbeam,
                          FeatureScorer* scorer);
  void makeT0cutoffBeam(u32 fullAnalysis, u32 rightBeam);
  bool patternIsStatic() const { return patternStatic_ != nullptr; }
  void computeUniOnlyPatterns(i32 bndIdx,
                              features::impl::PrimitiveFeatureContext* pfc);
  void adjustBeamScores(util::ArraySlice<float> scoreWeights);
  void remakeEosBeam(util::ArraySlice<float> scoreWeights);
};

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_SCORE_PROCESSOR_H
