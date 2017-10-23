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
  u32 maxEnds = 0;
  for (u32 idx = 2; idx < lattice_->createdBoundaryCount(); ++idx) {
    auto bnd = lattice_->boundary(idx);
    maxNodes = std::max<u32>(maxNodes, bnd->localNodeCount());
    maxEnds = std::max<u32>(maxEnds, bnd->ends()->arraySize());
  }
  this->runStats_.maxStarts = maxNodes;
  this->runStats_.maxEnds = maxEnds;

  auto *alloc = analyzer->alloc();
  scores_.prepare(alloc, maxNodes);
  beamCandidates_ = alloc->allocateBuf<BeamCandidate>(
      maxEnds * lattice_->config().beamSize, 64);
  analyzer->core().features().ngramPartial->allocateBuffers(&featureBuffer_,
                                                            runStats_, alloc);
}

void ScoreProcessor::copyFeatureScores(i32 left, i32 beam,
                                       LatticeBoundaryScores *bndconn) {
  bndconn->importBeamScore(left, 0, beam, scores_.bufferT2());
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

u32 fillBeamCandidates(Lattice *l, LatticeBoundary *bnd, NodeScores scores,
                       const ScorerDef *pDef,
                       util::MutableArraySlice<BeamCandidate> cands) {
  auto &weights = pDef->scoreWeights;
  JPP_DCHECK_EQ(scores.numScorers(), weights.size());
  auto ends = bnd->ends();
  u32 activeBeams = 0;
  switch (scores.numScorers()) {  // unroll loop for weights for common cases
    case 2: {
      auto w0 = weights[0];
      auto w1 = weights[1];
      for (u16 left = 0; left < scores.left(); ++left) {
        auto leftPtr = ends->nodePtrs().at(left);
        auto leftBeam = l->boundary(leftPtr.boundary)
                            ->starts()
                            ->beamData()
                            .row(leftPtr.position);
        for (u16 beam = 0; beam < scores.beam(); ++beam) {
          auto &leftElm = leftBeam.at(beam);
          if (EntryBeam::isFake(leftElm)) {
            break;
          }
          auto s = scores.beamLeft(beam, left);
          auto localScore = s.at(0) * w0 + s.at(1) + w1;
          auto score = leftElm.totalScore + localScore;
          cands.at(activeBeams) = BeamCandidate{score, left, beam};
          activeBeams += 1;
        }
      }
      break;
    }
    case 1: {
      auto w0 = weights[0];
      for (u16 left = 0; left < scores.left(); ++left) {
        auto leftPtr = ends->nodePtrs().at(left);
        auto leftBeam = l->boundary(leftPtr.boundary)
                            ->starts()
                            ->beamData()
                            .row(leftPtr.position);
        for (u16 beam = 0; beam < scores.beam(); ++beam) {
          auto &leftElm = leftBeam.at(beam);
          if (EntryBeam::isFake(leftElm)) {
            break;
          }
          auto s = scores.beamLeft(beam, left);
          auto localScore = s.at(0) * w0;
          auto score = leftElm.totalScore + localScore;
          cands.at(activeBeams) = BeamCandidate{score, left, beam};
          activeBeams += 1;
        }
      }
      break;
    }
    default: {
      for (u16 left = 0; left < scores.left(); ++left) {
        auto leftPtr = ends->nodePtrs().at(left);
        auto leftBeam = l->boundary(leftPtr.boundary)
                            ->starts()
                            ->beamData()
                            .row(leftPtr.position);
        for (u16 beam = 0; beam < scores.beam(); ++beam) {
          auto &leftElm = leftBeam.at(beam);
          if (EntryBeam::isFake(leftElm)) {
            break;
          }
          auto s = scores.beamLeft(beam, left);
          Score score = leftElm.totalScore;
          for (int i = 0; i < s.size(); ++i) {
            score += s.at(i) * weights[i];
          }
          cands.at(activeBeams) = BeamCandidate{score, left, beam};
          activeBeams += 1;
        }
      }
    }
  }
  return activeBeams;
}

util::ArraySlice<BeamCandidate> processBeamCandidates(
    util::MutableArraySlice<BeamCandidate> candidates, u32 maxBeam) {
  auto comp = [](const BeamCandidate &c1, const BeamCandidate &c2) {
    return c1.score > c2.score;
  };
  if (candidates.size() > maxBeam * 2) {
    u32 maxElems = maxBeam * 2;
    auto iter = util::partition(candidates.begin(), candidates.end(), comp,
                                maxBeam, maxElems);
    u32 sz = static_cast<u32>(iter - candidates.begin());
    candidates = util::MutableArraySlice<BeamCandidate>{candidates, 0, sz};
  }
  std::sort(candidates.begin(), candidates.end(), comp);
  auto size = std::min<u64>(maxBeam, candidates.size());
  return util::ArraySlice<BeamCandidate>{candidates, 0, size};
}

void ScoreProcessor::makeBeams(i32 boundary, LatticeBoundary *bnd,
                               const ScorerDef *sc) {
  auto myNodes = bnd->localNodeCount();
  auto prevData = bnd->ends()->nodePtrs();
  auto maxBeam = lattice_->config().beamSize;
  util::MutableArraySlice<BeamCandidate> cands{beamCandidates_, 0,
                                               maxBeam * prevData.size()};
  auto scores = bnd->scores();
  auto beamData = bnd->starts()->beamData();
  for (int node = 0; node < myNodes; ++node) {
    auto cnt =
        fillBeamCandidates(lattice_, bnd, scores->nodeScores(node), sc, cands);
    util::MutableArraySlice<BeamCandidate> candSlice{cands, 0, cnt};
    auto res = processBeamCandidates(candSlice, maxBeam);

    auto beamElems = beamData.row(node);
    // fill the beam
    u16 beam = 0;
    for (; beam < res.size(); ++beam) {
      auto beamCand = res.at(beam);
      auto prevPtr = prevData.at(beamCand.left());
      auto prevBnd = lattice_->boundary(prevPtr.boundary);
      auto prevNode = prevBnd->starts()->beamData().row(prevPtr.position);
      ConnectionPtr ptr{static_cast<u16>(boundary), beamCand.left(),
                        static_cast<u16>(node), beamCand.beam(),
                        &prevNode.at(beamCand.beam()).ptr};
      beamElems.at(beam) = ConnectionBeamElement{ptr, beamCand.score};
    }
    for (; beam < maxBeam; ++beam) {
      beamElems.at(beam) = EntryBeam::fake();
    }
  }
}

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp