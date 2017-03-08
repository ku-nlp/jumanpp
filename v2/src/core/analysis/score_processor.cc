//
// Created by Arseny Tolmachev on 2017/03/07.
//

#include "score_processor.h"
#include "core/analysis/lattice_types.h"
#include "core/impl/feature_impl_types.h"
#include "util/stl_util.h"

namespace jumanpp {
namespace core {
namespace analysis {

ScoreProcessor *ScoreProcessor::make(
    Lattice *lattice, util::memory::ManagedAllocatorCore *alloc) {
  auto numFeatures = lattice->config().numFinalFeatures;
  auto patFeatures = lattice->config().numFeaturePatterns;
  auto beamsize = lattice->config().beamSize;
  u32 maxstart = 0;

  auto bcnt = lattice->createdBoundaryCount();
  for (int i = 0; i < bcnt; ++i) {
    auto bnd = lattice->boundary(i);
    maxstart = std::max(maxstart, bnd->localNodeCount());
  }

  auto t2fbuffer = alloc->allocateBuf<u64>(patFeatures * beamsize);
  auto scoreBuf = alloc->allocateBuf<Score>(maxstart * beamsize);
  auto connBuf = alloc->allocateBuf<ConnectionBeamElement>(beamsize);
  auto ngramBuf = alloc->allocateBuf<u32>(numFeatures * maxstart);
  util::Sliceable<u64> t2sl{t2fbuffer, numFeatures, beamsize};
  util::Sliceable<Score> scoreSl{scoreBuf, maxstart, beamsize};
  util::Sliceable<u32> ngramSl{ngramBuf, numFeatures, maxstart};
  return alloc->make<ScoreProcessor>(lattice, t2sl, scoreSl, connBuf, ngramSl);
}

ScoreProcessor::ScoreProcessor(
    Lattice *lattice_, const util::Sliceable<u64> &t2features,
    const util::Sliceable<Score> &scoreBuffer,
    const util::MutableArraySlice<ConnectionBeamElement> &beamPtrs,
    const util::Sliceable<u32> &ngramFeatures)
    : lattice_(lattice_),
      t2features(t2features),
      scoreBuffer(scoreBuffer),
      beamPtrs(beamPtrs),
      ngramFeatures(ngramFeatures) {}

void ScoreProcessor::computeNgramFeatures(
    i32 beamIdx, const features::FeatureHolder &features,
    util::Sliceable<u64> t0features, util::ArraySlice<u64> t1features) {
  auto ngf = ngramFeatures.topRows(t0features.numRows());
  features::impl::NgramFeatureData nfd{ngf, t2features.row(beamIdx), t1features,
                                       t0features};
  features.ngram->applyBatch(&nfd);
}

void ScoreProcessor::updateBeams(i32 boundary, i32 endPos, LatticeBoundary *bnd,
                                 LatticeBoundaryConnection *bndconn,
                                 ScoreConfig *sc) {
  auto beam = bnd->starts()->beamData();
  u16 bnd16 = (u16)boundary;
  u16 end16 = (u16)endPos;
  auto &scoreWeights = sc->scoreWeights;
  for (int prevBeam = 0; prevBeam < beamSize_; ++prevBeam) {
    auto scoreData = bndconn->entryScores(prevBeam);
    for (i32 beginPos = 0; beginPos < beam.numRows(); ++beginPos) {
      auto itemBeam = beam.row(beginPos);
      auto scores = scoreData.row(beginPos);
      Score s = 0;
      for (int sw = 0; sw < scoreWeights.size(); ++sw) {
        s += scoreWeights[sw] * scores[sw];
      }
      auto& prevBi = this->beamPtrs[prevBeam];
      auto cumScore = s + prevBi.totalScore;
      ConnectionPtr cptr{bnd16, end16, ((u16)beginPos), (u16)prevBeam, &prevBi.ptr};
      ConnectionBeamElement cbe{cptr, cumScore};
      EntryBeam beamObj{itemBeam};
      beamObj.pushItem(cbe);
    }
  }
}

void ScoreProcessor::copyFeatureScores(LatticeBoundaryConnection *bndconn) {
  for (int beam = 0; beam < beamSize_; ++beam) {
    auto scores = scoreBuffer.row(beam);
    bndconn->importBeamScore(0, beam, scores);
  }
}

void ScoreProcessor::computeFeatureScores(i32 beamIdx, FeatureScorer *scorer,
                                          u32 sliceSize) {
  auto features = ngramFeatures.topRows(sliceSize);
  auto scores = scoreBuffer.row(beamIdx);
  scorer->compute(scores, features);
}

void ScoreProcessor::gatherT2Features(
    util::ArraySlice<ConnectionBeamElement> beam, Lattice &lattice) {
  beamSize_ = 0;
  for (auto &e : beam) {
    if (EntryBeam::isFake(e)) {
      break;
    }
    ++beamSize_;
  }
  beamPtrs = util::ArraySlice<ConnectionBeamElement>{beam, 0, (u32)beamSize_};

  for (int i = 0; i < beamSize_; ++i) {
    auto &ptr = beamPtrs[i];
    //get actual nodeptr
    auto bnd1 = lattice.boundary(ptr.ptr.boundary);
    auto nodePtr = bnd1->ends()->nodePtrs().at(ptr.ptr.left);
    //get pattern features
    auto bnd2 = lattice.boundary(nodePtr.boundary);
    auto patfeatures = bnd2->starts()->patternFeatureData().row(nodePtr.position);
    auto row = t2features.row(i);
    util::copy_buffer(patfeatures, row);
  }
}
}
}
}