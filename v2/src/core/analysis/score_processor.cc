//
// Created by Arseny Tolmachev on 2017/03/07.
//

#include "score_processor.h"
#include <numeric>
#include "core/analysis/analyzer_impl.h"
#include "core/analysis/lattice_types.h"
#include "core/impl/feature_impl_types.h"
#include "util/logging.hpp"

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
      lattice_{analyzer->lattice()},
      t1PtrData_{analyzer->alloc()} {
  u32 maxNodes = 0;
  u32 maxEnds = 0;
  for (u32 idx = 2; idx < lattice_->createdBoundaryCount(); ++idx) {
    auto bnd = lattice_->boundary(idx);
    maxNodes = std::max<u32>(maxNodes, bnd->localNodeCount());
    maxEnds = std::max<u32>(maxEnds, bnd->ends()->arraySize());
  }
  this->runStats_.maxStarts = maxNodes;
  this->runStats_.maxEnds = maxEnds;
  this->globalBeamSize_ = analyzer->cfg().globalBeamSize;

  auto *alloc = analyzer->alloc();
  scores_.prepare(alloc, maxNodes);
  auto &lcfg = lattice_->config();
  beamCandidates_ =
      alloc->allocateBuf<BeamCandidate>(maxEnds * lcfg.beamSize, 64);

  analyzer->core().features().ngramPartial->allocateBuffers(&featureBuffer_,
                                                            runStats_, alloc);
  globalBeamSize_ = analyzer->cfg().globalBeamSize;
  t1PtrData_.reserve(globalBeamSize_);
  globalBeam_ = alloc->allocateBuf<BeamCandidate>(maxEnds * lcfg.beamSize, 64);
  t1positions_ = alloc->allocateBuf<u32>(globalBeamSize_);
  t1patBuf_ = alloc->allocate2d<u64>(maxEnds, lcfg.numFeaturePatterns);
  t2patBuf_ = alloc->allocate2d<u64>(globalBeamSize_, lcfg.numFeaturePatterns);
  gbeamScoreBuf_ = alloc->allocateBuf<Score>(globalBeamSize_);
  beamIdxBuffer_ = alloc->allocateBuf<u32>(globalBeamSize_);
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
  auto comp = std::greater<>();
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
      beamElems.at(beam) = ConnectionBeamElement{ptr, beamCand.score()};
    }
    for (; beam < maxBeam; ++beam) {
      beamElems.at(beam) = EntryBeam::fake();
    }
  }
}

util::ArraySlice<BeamCandidate> ScoreProcessor::makeGlobalBeam(i32 bndIdx,
                                                               i32 maxElems) {
  auto bnd = lattice_->boundary(bndIdx);
  auto ends = bnd->ends();
  u32 count = 0;
  util::ArraySlice<LatticeNodePtr> leftNodes = ends->nodePtrs();
  for (u16 left = 0; left < leftNodes.size(); ++left) {
    auto ptr = leftNodes.at(left);
    auto endBnd = lattice_->boundary(ptr.boundary);
    auto endBeam = endBnd->starts()->beamData().row(ptr.position);
    u16 beamIdx = 0;
    for (auto &el : endBeam) {
      if (EntryBeam::isFake(el)) {
        break;
      }
      globalBeam_[count] = BeamCandidate{el.totalScore, left, beamIdx};
      ++count;
      ++beamIdx;
    }
  }
  util::MutableArraySlice<BeamCandidate> slice{globalBeam_, 0, count};
  auto res = processBeamCandidates(slice, maxElems);
  return res;
}

void ScoreProcessor::computeGbeamScores(i32 bndIdx,
                                        util::ArraySlice<BeamCandidate> gbeam,
                                        FeatureScorer *features) {
  auto bnd = lattice_->boundary(bndIdx);
  auto t1Ptrs = dedupT1(bndIdx, gbeam);
  util::Sliceable<u64> t1data = gatherT1();
  util::Sliceable<u64> t2data = gatherT2(bndIdx, gbeam);

  auto right = bnd->starts();
  auto t0data = right->patternFeatureData();
  util::MutableArraySlice<Score> result{gbeamScoreBuf_, 0, gbeam.size()};
  for (auto t0idx = 0; t0idx < t0data.numRows(); ++t0idx) {
    JPP_CAPTURE(t0idx);
    auto t0 = t0data.row(t0idx);
    ngramApply_->applyBiTri(&featureBuffer_, t0, t1data, t2data, t1Ptrs,
                            features, result);
    auto t0Score = scores_.bufferT0().at(t0idx);
    copyT0Scores(bndIdx, t0idx, gbeam, result, t0Score);
    makeT0Beam(bndIdx, t0idx, gbeam, result);
  }
}

util::ArraySlice<u32> ScoreProcessor::dedupT1(
    i32 bndIdx, util::ArraySlice<BeamCandidate> gbeam) {
  auto left = lattice_->boundary(bndIdx)->ends()->nodePtrs();
  t1PtrData_.clear();

  util::MutableArraySlice<u32> subset{t1positions_, 0, gbeam.size()};
  for (int i = 0; i < gbeam.size(); ++i) {
    auto &it = left.at(gbeam.at(i).left());
    u32 curSize = static_cast<u32>(t1PtrData_.size());
    auto idx = t1PtrData_.findOrInsert(it, [curSize]() { return curSize; });
    subset[i] = idx;
  }

  return subset;
}

util::Sliceable<u64> ScoreProcessor::gatherT1() {
  util::Sliceable<u64> result = t1patBuf_.topRows(t1PtrData_.size());
  for (auto &obj : t1PtrData_) {
    auto bnd = lattice_->boundary(obj.first.boundary);
    const auto pats = bnd->starts()->patternFeatureData();
    auto therow = pats.row(obj.first.position);
    auto target = result.row(obj.second);
    util::copy_buffer(therow, target);
  }
  return result;
}

util::Sliceable<u64> ScoreProcessor::gatherT2(
    i32 bndIdx, util::ArraySlice<BeamCandidate> gbeam) {
  util::Sliceable<u64> result = t2patBuf_.topRows(gbeam.size());
  auto ptrs = lattice_->boundary(bndIdx)->ends()->nodePtrs();
  int i = 0;
  for (auto &c : gbeam) {
    auto pt = ptrs.at(c.left());
    auto bnd = lattice_->boundary(pt.boundary);
    auto beam = bnd->starts()->beamData().row(pt.position);
    auto &beamPtr = beam.at(c.beam());
    auto prev = beamPtr.ptr.previous;
    auto t2Bnd = lattice_->boundary(prev->boundary)->starts();
    auto therow = t2Bnd->patternFeatureData().row(prev->right);
    auto target = result.row(i);
    util::copy_buffer(therow, target);
    ++i;
  }
  return result;
}

void ScoreProcessor::copyT0Scores(i32 bndIdx, i32 t0idx,
                                  util::ArraySlice<BeamCandidate> gbeam,
                                  util::MutableArraySlice<Score> scores,
                                  Score t0Score) {
  auto sholder = lattice_->boundary(bndIdx)->scores();
  auto nscores = sholder->nodeScores(t0idx);
  for (int i = 0; i < gbeam.size(); ++i) {
    auto &v = scores.at(i);
    v += t0Score;
    auto &gb = gbeam.at(i);
    nscores.beamLeft(gb.beam(), gb.left()).at(0) = v;
  }
}

void ScoreProcessor::makeT0Beam(i32 bndIdx, i32 t0idx,
                                util::ArraySlice<BeamCandidate> gbeam,
                                util::MutableArraySlice<Score> scores) {
  auto maxBeam = lattice_->config().beamSize;
  util::MutableArraySlice<u32> idxes{beamIdxBuffer_, 0, gbeam.size()};
  std::iota(idxes.begin(), idxes.end(), 0);
  auto comp = [&scores](u32 i1, u32 i2) { return scores[i1] > scores[i2]; };
  auto itr = idxes.end();
  auto partitionBoundary = maxBeam * 2;
  if (idxes.size() > partitionBoundary) {
    itr = util::partition(idxes.begin(), itr, comp, maxBeam, partitionBoundary);
  }
  std::sort(idxes.begin(), itr, comp);

  auto start = lattice_->boundary(bndIdx)->starts();
  auto beam = start->beamData().row(t0idx);
  auto prevPtrs = lattice_->boundary(bndIdx)->ends()->nodePtrs();

  u32 beamIdx = 0;
  for (auto it = idxes.begin(); it < itr; ++it) {
    if (beamIdx >= maxBeam) {
      break;
    }
    auto &prev = gbeam.at(*it);
    auto localScore = scores.at(*it);
    auto globalScore = localScore + prev.score();
    auto prevPtr = prevPtrs.at(prev.left());
    auto prevBnd = lattice_->boundary(prevPtr.boundary)->starts();
    auto &prevFullPtr =
        prevBnd->beamData().row(prevPtr.position).at(prev.beam());
    ConnectionPtr cp{static_cast<u16>(bndIdx), prev.left(),
                     static_cast<u16>(t0idx), prev.beam(), &prevFullPtr.ptr};

    ConnectionBeamElement cbe{cp, globalScore};

    // LOG_TRACE() << "assign beam " << cbe << " at " << bndIdx << ", " <<
    // t0idx;
    beam.at(beamIdx) = cbe;

    ++beamIdx;
  }

  for (; beamIdx < maxBeam; ++beamIdx) {
    beam.at(beamIdx) = EntryBeam::fake();
  }
}

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp