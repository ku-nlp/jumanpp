//
// Created by Arseny Tolmachev on 2017/03/07.
//

#include "score_processor.h"
#include "core/analysis/analyzer_impl.h"
#include "core/analysis/lattice_types.h"
#include "core/impl/feature_impl_types.h"
#include "util/stl_util.h"

namespace jumanpp {
namespace core {
namespace analysis {

std::pair<Status, ScoreProcessor *> ScoreProcessor::make(AnalyzerImpl *impl) {
  auto alloc = impl->alloc();
  auto procBuf = alloc->allocate<ScoreProcessor>();
  auto proc = new (procBuf) ScoreProcessor{impl};
  Status s = proc->ngram_.initialize(impl);
  if (s) {
    proc->scores_.prepare(impl, proc->ngram_.maxStarts());
  }
  return std::make_pair(std::move(s), proc);
}

ScoreProcessor::ScoreProcessor(AnalyzerImpl *analyzer)
    : lattice_{analyzer->lattice()}, ngram_{analyzer} {}

void ScoreProcessor::updateBeams(i32 boundary, i32 endPos, LatticeBoundary *bnd,
                                 LatticeBoundaryConnection *bndconn,
                                 const ScorerDef *sc) {
  auto beam = bnd->starts()->beamData();
  u16 bnd16 = (u16)boundary;
  u16 end16 = (u16)endPos;
  auto &scoreWeights = sc->scoreWeights;
  auto ptr = bnd->ends()->nodePtrs().at(endPos);
  auto swsize = scoreWeights.size();
  resolveBeamAt(ptr.boundary, ptr.position);
  for (int prevBeam = 0; prevBeam < beamSize_; ++prevBeam) {
    auto scoreData = bndconn->entryScores(prevBeam);
    for (i32 beginPos = 0; beginPos < beam.numRows(); ++beginPos) {
      auto itemBeam = beam.row(beginPos);
      auto scores = scoreData.row(beginPos);
      Score s = 0;
      switch (swsize) {
        case 2:
          s += scoreWeights[1] * scores[1];
        case 1:
          s += scoreWeights[0] * scores[0];
          break;
        default:
          for (int sw = 0; sw < swsize; ++sw) {
            s += scoreWeights[sw] * scores[sw];
          }
      }
      auto &prevBi = this->beamPtrs_[prevBeam];
      auto cumScore = s + prevBi.totalScore;
      ConnectionPtr cptr{bnd16, end16, ((u16)beginPos), (u16)prevBeam,
                         &prevBi.ptr};
      ConnectionBeamElement cbe{cptr, cumScore};
      EntryBeam beamObj{itemBeam};
      beamObj.pushItem(cbe);
    }
  }
}

void ScoreProcessor::copyFeatureScores(i32 beam,
                                       LatticeBoundaryConnection *bndconn) {
  bndconn->importBeamScore(0, beam, scores_.bufferT2());
}

void ScoreProcessor::resolveBeamAt(i32 boundary, i32 position) {
  auto bnd = lattice_->boundary(boundary);
  auto startData = bnd->starts();
  auto beam = startData->beamData().row(position);
  beamSize_ = 0;
  for (auto &e : beam) {
    if (EntryBeam::isFake(e)) {
      break;
    }
    ++beamSize_;
  }
  beamPtrs_ = util::ArraySlice<ConnectionBeamElement>{beam, 0, (u32)beamSize_};
}

ConnectionPtr *ScoreProcessor::realPtr(const ConnectionPtr &ptr) const {
  auto bnd = lattice_->boundary(ptr.boundary);
  auto itemPtr = bnd->ends()->nodePtrs().at(ptr.left);
  auto bnd2 = lattice_->boundary(itemPtr.boundary);
  auto beam = bnd2->starts()->beamData().row(itemPtr.position);
  return &beam.at(ptr.beam).ptr;
}

void ScoreProcessor::startBoundary(u32 currentNodes) {
  scores_.newBoundary(currentNodes);
}

void ScoreProcessor::applyT0(i32 boundary, FeatureScorer *features) {
  ngram_.computeUnigrams(boundary);
  ngram_.setScores(features, scores_.bufferT0());
  ngram_.computeBigramsStep1(boundary);
  ngram_.computeTrigramsStep1(boundary);
}

void ScoreProcessor::applyT1(i32 boundary, i32 position,
                             FeatureScorer *features) {
  ngram_.computeBigramsStep2(boundary, position);
  ngram_.addScores(features, scores_.bufferT0(), scores_.bufferT1());
  ngram_.computeTrigramsStep2(boundary, position);
}

void ScoreProcessor::applyT2(i32 beamIdx, FeatureScorer *features) {
  auto &beam = beamPtrs_.at(beamIdx);
  auto &ptr = beam.ptr;
  ngram_.compute3GramsStep3(ptr.boundary, ptr.right);
  ngram_.addScores(features, scores_.bufferT1(), scores_.bufferT2());
}

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp