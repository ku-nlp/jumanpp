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
  return std::make_pair(Status::Ok(), proc);
}

ScoreProcessor::ScoreProcessor(AnalyzerImpl *analyzer)
    : ngramApply_{analyzer->core().features().ngramPartial},
      lattice_{analyzer->lattice()} {
  u32 maxNodes = 0;
  for (u32 idx = 2; idx < lattice_->createdBoundaryCount(); ++idx) {
    auto bnd = lattice_->boundary(idx);
    maxNodes = std::max<u32>(maxNodes, bnd->localNodeCount());
  }
  this->runStats_.maxStarts = maxNodes;

  auto *alloc = analyzer->alloc();
  scores_.prepare(alloc, maxNodes);
  analyzer->core().features().ngramPartial->allocateBuffers(&featureBuffer_,
                                                            runStats_, alloc);
}

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

void ScoreProcessor::startBoundary(u32 currentNodes) {
  scores_.newBoundary(currentNodes);
}

void ScoreProcessor::applyT0(i32 boundary, FeatureScorer *features) {
  auto patterns = lattice_->boundary(boundary)->starts()->patternFeatureData();
  ngramApply_->applyUni(&featureBuffer_, patterns, features,
                        scores_.bufferT0());
  ngramApply_->applyBiStep1(&featureBuffer_, patterns);
  ngramApply_->applyTriStep1(&featureBuffer_, patterns);
}

void ScoreProcessor::applyT1(i32 boundary, i32 position,
                             FeatureScorer *features) {
  auto result = scores_.bufferT1();
  util::copy_buffer(scores_.bufferT0(), result);
  auto item = lattice_->boundary(boundary)->starts()->patternFeatureData().row(
      position);
  ngramApply_->applyBiStep2(&featureBuffer_, item, features, result);
  ngramApply_->applyTriStep2(&featureBuffer_, item);
}

void ScoreProcessor::applyT2(i32 beamIdx, FeatureScorer *features) {
  auto &beam = beamPtrs_.at(beamIdx);
  auto ptr = beam.ptr.previous;
  auto item = lattice_->boundary(ptr->boundary)
                  ->starts()
                  ->patternFeatureData()
                  .row(ptr->right);
  auto result = scores_.bufferT2();
  util::copy_buffer(scores_.bufferT1(), result);
  ngramApply_->applyTriStep3(&featureBuffer_, item, features, result);
}

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp