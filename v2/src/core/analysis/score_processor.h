//
// Created by Arseny Tolmachev on 2017/03/07.
//

#ifndef JUMANPP_SCORE_PROCESSOR_H
#define JUMANPP_SCORE_PROCESSOR_H

#include "core/analysis/lattice_config.h"
#include "core/analysis/score_api.h"
#include "core/features_api.h"
#include "util/memory.hpp"

namespace jumanpp {
namespace core {
namespace analysis {

class Lattice;
class LatticeBoundary;
class LatticeBoundaryConnection;

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
  util::Sliceable<u64> t2features;     // pattern x beamsize
  util::Sliceable<Score> scoreBuffer;  // maxstart x beamsize
  util::Sliceable<u32> ngramFeatures;  // features x maxstart
  //is set to the actual beam by gatherT2features
  util::ArraySlice<ConnectionBeamElement> beamPtrs;
  i32 beamSize_;

 public:
  ScoreProcessor(Lattice* lattice_, const util::Sliceable<u64>& t2features,
                 const util::Sliceable<Score>& scoreBuffer,
                 const util::MutableArraySlice<ConnectionBeamElement>& beamPtrs,
                 const util::Sliceable<u32>& ngramFeatures);

  static ScoreProcessor* make(Lattice* lattice,
                              util::memory::ManagedAllocatorCore* alloc);

  i32 activeBeamSize() const { return beamSize_; }

  void gatherT2Features(util::ArraySlice<ConnectionBeamElement> beam,
                        Lattice& lattice);

  void computeNgramFeatures(i32 beamIdx,
                            const features::FeatureHolder& features,
                            util::Sliceable<u64> t0features,
                            util::ArraySlice<u64> t1features);

  void computeFeatureScores(i32 beamIdx, FeatureScorer* scorer, u32 sliceSize);

  void copyFeatureScores(LatticeBoundaryConnection* bndconn);

  void updateBeams(i32 boundary, i32 endPos, LatticeBoundary *bnd,
                   LatticeBoundaryConnection *bndconn, ScoreConfig *sc);
};

}  // analysis
}  // core
}  // jumanpp

#endif  // JUMANPP_SCORE_PROCESSOR_H
