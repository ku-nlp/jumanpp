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
      cfg_{&analyzer->cfg()},
      t1PtrData_{analyzer->alloc()} {
  u32 maxNodes = 0;
  u32 maxEnds = 0;
  for (u32 idx = 2; idx < lattice_->createdBoundaryCount(); ++idx) {
    auto bnd = lattice_->boundary(idx);
    maxNodes = std::max<u32>(maxNodes, bnd->localNodeCount());
    maxEnds = std::max<u32>(maxEnds, bnd->ends()->nodePtrs().size());
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
  util::fill(featureBuffer_.valueBuffer1, 0);

  globalBeamSize_ = analyzer->cfg().globalBeamSize;
  if (globalBeamSize_ > 0) {
    t1PtrData_.reserve(globalBeamSize_);
    globalBeam_ =
        alloc->allocateBuf<BeamCandidate>(maxEnds * lcfg.beamSize, 64);
    const u64 *gbeamPtr = reinterpret_cast<const u64 *>(globalBeam_.data());
    lattice_->setLastGbeam(gbeamPtr);
    t1positions_ = alloc->allocateBuf<u32>(globalBeamSize_);
    t1patBuf_ = alloc->allocate2d<u64>(maxEnds, lcfg.numFeaturePatterns);
    t2patBuf_ =
        alloc->allocate2d<u64>(globalBeamSize_, lcfg.numFeaturePatterns);
    gbeamScoreBuf_ = alloc->allocateBuf<Score>(globalBeamSize_);
    beamIdxBuffer_ = alloc->allocateBuf<u32>(globalBeamSize_);
    if (cfg_->rightGbeamCheck > 0) {
      t0prescores_ =
          alloc->allocate2d<Score>(cfg_->rightGbeamCheck, maxNodes, 64);
      t0cutoffBuffer_ = alloc->allocateBuf<Score>(maxNodes);
      t0cutoffIdxBuffer_ = alloc->allocateBuf<u32>(maxNodes);
    }
  }

  patternStatic_ = analyzer->core().features().patternStatic.get();
  patternDynamic_ = analyzer->core().features().patternDynamic.get();
  plugin_ = analyzer->plugin();
}

void ScoreProcessor::copyFeatureScores(i32 left, i32 beam,
                                       LatticeBoundaryScores *bndconn) {
  bndconn->importBeamScore(left, 0, beam, scores_.bufferT2());
}

void ScoreProcessor::applyPluginToFullBeam(i32 bndNum, i32 left, i32 beam) {
  if (!plugin_) {
    return;
  }

  auto &prev = beamPtrs_.at(beam);
  auto score = scores_.bufferT2();

  for (u16 t0 = 0; t0 < score.size(); ++t0) {
    ConnectionPtr ptr{static_cast<u16>(bndNum), static_cast<u16>(left), t0,
                      static_cast<u16>(beam), &prev.ptr};

    plugin_->updateScore(lattice_, ptr, &score.at(t0));
  }
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

void ScoreProcessor::computeT0All(
    i32 boundary, FeatureScorer *features,
    features::impl::PrimitiveFeatureContext *pfc) {
  auto bnd = lattice_->boundary(boundary)->starts();
  auto nodeInfo = bnd->nodeInfo();
  auto nodeFeatures = bnd->entryData();
  auto scores = scores_.bufferT0();
  featureBuffer_.currentElems = static_cast<u32>(bnd->numEntries());
  patternStatic_->patternsAndUnigramsApply(
      pfc, nodeInfo, nodeFeatures, &featureBuffer_, bnd->patternFeatureData(),
      features, scores);
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

namespace {

u32 fillBeamCandidates(Lattice *l, LatticeBoundary *bnd, NodeScores scores,
                       const ScorerDef *pDef,
                       util::MutableArraySlice<BeamCandidate> cands) {
  auto &weights = pDef->scoreWeights;
  JPP_DCHECK_EQ(scores.numScorers(), weights.size());
  auto ends = bnd->ends();
  u32 activeBeams = 0;
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
      auto localScore = s.at(0);
      auto score = leftElm.totalScore + localScore;
      cands.at(activeBeams) = BeamCandidate{score, left, beam};
      activeBeams += 1;
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

}  // namespace

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
    size_t rest = maxBeam - beam;
    if (rest > 0) {
      std::memset(&beamElems.at(beam), 0xff,
                  rest * sizeof(ConnectionBeamElement));
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
  auto gbptrs = ends->globalBeam();
  if (gbptrs.size() > 0) {
    JPP_DCHECK_LE(res.size(), gbptrs.size());
    ends->setGlobalBeamSize(res.size());
    for (int i = 0; i < res.size(); ++i) {
      auto beamEl = res.at(i);
      auto left = leftNodes.at(beamEl.left());
      auto nodeBnd = lattice_->boundary(left.boundary);
      auto beams = nodeBnd->starts()->beamData();
      gbptrs.at(i) = &beams.row(left.position).at(beamEl.beam());
    }
  }
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

  if (cfg_->rightGbeamCheck > 0) {
    // we cut off right elements as well

    auto size = static_cast<size_t>(cfg_->rightGbeamCheck);
    auto fullBeamApplySize =
        std::min<size_t>({size, bnd->localNodeCount(), gbeam.size()});
    auto toKeep = std::min<u32>(static_cast<u32>(cfg_->rightGbeamSize),
                                bnd->localNodeCount());
    u32 remainingItems = std::max<u32>(gbeam.size() - fullBeamApplySize, 0);
    auto t1PtrTail =
        util::ArraySlice<u32>{t1Ptrs, fullBeamApplySize, remainingItems};
    auto t2Tail = t2data.rows(fullBeamApplySize, t2data.numRows());
    util::MutableArraySlice<Score> resultTail{result, fullBeamApplySize,
                                              remainingItems};
    util::ArraySlice<BeamCandidate> gbeamHead{gbeam, 0, fullBeamApplySize};
    util::ArraySlice<BeamCandidate> gbeamTail{gbeam, fullBeamApplySize,
                                              remainingItems};

    computeT0Prescores(gbeam, features);
    applyPluginToPrescores(bndIdx, gbeamHead);
    makeT0cutoffBeam(static_cast<u32>(fullBeamApplySize), toKeep);

    auto t0pos = 0;
    // first, we process elements which require feature/score computation
    for (; t0pos < toKeep; ++t0pos) {
      auto t0idx = t0cutoffIdxBuffer_.at(t0pos);
      auto t0 = t0data.row(t0idx);
      for (int i = 0; i < fullBeamApplySize; ++i) {
        result.at(i) = t0prescores_.row(i).at(t0idx);
      }
      copyT0Scores(bndIdx, t0idx, gbeamHead, result, 0);
      if (t1PtrTail.size() > 0) {
        auto t0Score = scores_.bufferT0().at(t0idx);
        ngramApply_->applyBiTri(&featureBuffer_, t0idx, t0, t1data, t2Tail,
                                t1PtrTail, features, resultTail);
        applyPluginToGbeam(bndIdx, t0idx, gbeamTail, resultTail);
        copyT0Scores(bndIdx, t0idx, gbeamTail, resultTail, t0Score);
      }
      makeT0Beam(bndIdx, t0idx, gbeam, result);
    }

    // then we form beams for the remaining items
    for (; t0pos < t0data.numRows(); ++t0pos) {
      auto t0idx = t0cutoffIdxBuffer_.at(t0pos);
      for (int i = 0; i < fullBeamApplySize; ++i) {
        result.at(i) = t0prescores_.row(i).at(t0idx);
      }
      copyT0Scores(bndIdx, t0idx, gbeamHead, result, 0);
      makeT0Beam(bndIdx, t0idx, gbeamHead, result);
    }

  } else {
    // we score all gbeam <-> right pairs
    for (auto t0idx = 0; t0idx < t0data.numRows(); ++t0idx) {
      JPP_CAPTURE(t0idx);
      auto t0 = t0data.row(t0idx);
      ngramApply_->applyBiTri(&featureBuffer_, t0idx, t0, t1data, t2data,
                              t1Ptrs, features, result);
      auto t0Score = scores_.bufferT0().at(t0idx);
      applyPluginToGbeam(bndIdx, t0idx, gbeam, result);
      copyT0Scores(bndIdx, t0idx, gbeam, result, t0Score);
      makeT0Beam(bndIdx, t0idx, gbeam, result);
    }
  }
}

util::ArraySlice<u32> ScoreProcessor::dedupT1(
    i32 bndIdx, util::ArraySlice<BeamCandidate> gbeam) {
  auto left = lattice_->boundary(bndIdx)->ends()->nodePtrs();
  t1PtrData_.clear_no_resize();

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
    v += gb.score();
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
  auto partitionBoundary = maxBeam * 4 / 3;
  if (idxes.size() > partitionBoundary) {
    itr = util::partition(idxes.begin(), itr, comp, maxBeam, partitionBoundary);
  }
  std::sort(idxes.begin(), itr, comp);

  auto start = lattice_->boundary(bndIdx)->starts();
  auto beam = start->beamData().row(t0idx);
  const auto ends = lattice_->boundary(bndIdx)->ends();
  auto gbeamNodes = ends->globalBeam();

  u32 beamIdx = 0;
  for (auto it = idxes.begin(); it < itr; ++it) {
    if (beamIdx >= maxBeam) {
      break;
    }
    auto &prev = gbeam.at(*it);
    auto prevPtr = gbeamNodes.at(*it);
    auto globalScore = scores.at(*it);
    ConnectionPtr cp{static_cast<u16>(bndIdx), prev.left(),
                     static_cast<u16>(t0idx), prev.beam(), &prevPtr->ptr};

    ConnectionBeamElement cbe{cp, globalScore};

    //   LOG_TRACE() << "assign beam " << cbe << " at " << bndIdx << ", " <<
    //   t0idx << " gb: " << *it << " -> " << beamIdx;
    beam.at(beamIdx) = cbe;

    ++beamIdx;
  }

  size_t rest = maxBeam - beamIdx;
  if (rest > 0) {
    std::memset(&beam.at(beamIdx), 0xff, rest * sizeof(ConnectionBeamElement));
  }
}

void ScoreProcessor::makeT0cutoffBeam(u32 fullAnalysis, u32 rightBeam) {
  auto slice = t0prescores_.topRows(fullAnalysis);
  auto curElemCnt = featureBuffer_.currentElems;

  util::MutableArraySlice<u32> idxBuf{t0cutoffIdxBuffer_, 0, curElemCnt};
  std::iota(idxBuf.begin(), idxBuf.end(), 0);

  if (curElemCnt <= rightBeam) {
    return;
  }

  util::MutableArraySlice<Score> cutoffScores{t0cutoffBuffer_, 0, curElemCnt};
  for (int i = 0; i < curElemCnt; ++i) {
    Score s = 0;
    for (int j = 0; j < fullAnalysis; ++j) {
      s += slice.row(j).at(i);
    }
    cutoffScores.at(i) = s;
  }
  auto comp = [&](u32 a, u32 b) {
    return cutoffScores.at(a) > cutoffScores.at(b);
  };
  std::nth_element(idxBuf.begin(), idxBuf.begin() + rightBeam, idxBuf.end(),
                   comp);
}

void ScoreProcessor::computeT0Prescores(util::ArraySlice<BeamCandidate> gbeam,
                                        FeatureScorer *scorer) {
  auto max = cfg_->rightGbeamCheck;
  auto theMax = std::min<i32>(max, gbeam.size());
  for (int i = 0; i < theMax; ++i) {
    auto t1idx = t1positions_.at(i);
    auto t1 = t1patBuf_.row(t1idx);
    auto t2 = t2patBuf_.row(i);
    auto scores = t0prescores_.row(i);
    util::copy_buffer(scores_.bufferT0(), scores);
    ngramApply_->applyTriStep2(&featureBuffer_, t1);
    ngramApply_->applyBiStep2(&featureBuffer_, t1, scorer, scores);
    ngramApply_->applyTriStep3(&featureBuffer_, t2, scorer, scores);
  }
}

void ScoreProcessor::computeUniOnlyPatterns(
    i32 bndIdx, features::impl::PrimitiveFeatureContext *pfc) {
  auto bnd = lattice_->boundary(bndIdx)->starts();
  features::impl::PrimitiveFeatureData pfdata{bnd->nodeInfo(), bnd->entryData(),
                                              bnd->patternFeatureData()};
  patternDynamic_->applyUniOnly(pfc, &pfdata);
}

void ScoreProcessor::adjustBeamScores(util::ArraySlice<float> scoreWeights) {
  auto nbnd = lattice_->createdBoundaryCount();
  // going through global beam, it is on end side
  // so start iteration from the 3-rd boundary
  for (u32 latBnd = 3; latBnd < nbnd; ++latBnd) {
    auto curBnd = lattice_->boundary(latBnd);
    if (curBnd->localNodeCount() == 0) {
      continue;
    }
    auto end = curBnd->ends();
    auto gbeam = end->globalBeam();
    for (auto el : gbeam) {
      auto elScores = lattice_->boundary(el->ptr.boundary)->scores();
      auto nodeScores = elScores->nodeScores(el->ptr.right);
      auto scores = nodeScores.beamLeft(el->ptr.beam, el->ptr.left);
      float localScore = 0;
      for (int i = 0; i < scoreWeights.size(); ++i) {
        localScore += scores.at(i) * scoreWeights.at(i);
      }
      auto prevPtr = el->ptr.previous;
      // underlying object IS ConnectionBeamElement
      auto prevBeam = reinterpret_cast<const ConnectionBeamElement *>(prevPtr);
      localScore += prevBeam->totalScore;
      // underlying object is not const, so this is safe
      auto mutEL = const_cast<ConnectionBeamElement *>(el);
      mutEL->totalScore = localScore;
    }
  }
}

void ScoreProcessor::remakeEosBeam(util::ArraySlice<float> scoreWeights) {
  auto nbnd = lattice_->createdBoundaryCount();
  auto eosBnd = lattice_->boundary(nbnd - 1);
  auto rawGbeamData = lattice_->lastGbeamRaw();
  auto fullGbeam = eosBnd->ends()->globalBeam();
  util::MutableArraySlice<BeamCandidate> lastGbeam{
      reinterpret_cast<BeamCandidate *>(const_cast<u64 *>(rawGbeamData)),
      fullGbeam.size()};
  util::MutableArraySlice<Score> fullScores{gbeamScoreBuf_, 0,
                                            fullGbeam.size()};
  auto rawScores = eosBnd->scores()->nodeScores(0);
  for (int i = 0; i < fullGbeam.size(); ++i) {
    float localScore = 0;
    auto gbeamCand = lastGbeam.at(i);
    auto beamScore = fullGbeam.at(i)->totalScore;
    lastGbeam.at(i) =
        BeamCandidate{beamScore, gbeamCand.left(), gbeamCand.beam()};
    auto localScoreData =
        rawScores.beamLeft(gbeamCand.beam(), gbeamCand.left());
    for (int j = 0; j < scoreWeights.size(); ++j) {
      localScore += localScoreData.at(j) * scoreWeights.at(j);
    }
    fullScores.at(i) = localScore + beamScore;
  }
  makeT0Beam(nbnd - 1, 0, lastGbeam, fullScores);
}

void ScoreProcessor::applyPluginToPrescores(
    i32 bndIdx, util::ArraySlice<BeamCandidate> gbeam) {
  if (plugin_) {
    auto bnd = lattice_->boundary(bndIdx);
    auto t0num = bnd->localNodeCount();
    for (u32 t1i = 0; t1i < gbeam.size(); ++t1i) {
      auto scores = t0prescores_.row(t1i);
      auto &t1bc = gbeam.at(t1i);
      auto &t1nptr = bnd->ends()->nodePtrs().at(t1bc.left());
      auto t1bnd = lattice_->boundary(t1nptr.boundary)->starts();
      auto &t1beam = t1bnd->beamData().row(t1nptr.position).at(t1bc.beam());
      for (int t0 = 0; t0 < t0num; ++t0) {
        ConnectionPtr ptr{static_cast<u16>(bndIdx), t1bc.left(),
                          static_cast<u16>(t0), t1bc.beam(), &t1beam.ptr};
        plugin_->updateScore(lattice_, ptr, &scores.at(t0));
      }
    }
  }
}

void ScoreProcessor::applyPluginToGbeam(i32 bndIdx, i32 t0idx,
                                        util::ArraySlice<BeamCandidate> gbeam,
                                        util::MutableArraySlice<Score> scores) {
  if (plugin_) {
    auto bnd = lattice_->boundary(bndIdx);
    for (u32 t1i = 0; t1i < gbeam.size(); ++t1i) {
      auto &t1bc = gbeam.at(t1i);
      auto &t1nptr = bnd->ends()->nodePtrs().at(t1bc.left());
      auto t1bnd = lattice_->boundary(t1nptr.boundary)->starts();
      auto &t1beam = t1bnd->beamData().row(t1nptr.position).at(t1bc.beam());
      ConnectionPtr ptr{static_cast<u16>(bndIdx), t1bc.left(),
                        static_cast<u16>(t0idx), t1bc.beam(), &t1beam.ptr};
      plugin_->updateScore(lattice_, ptr, &scores.at(t1i));
    }
  }
}

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp