//
// Created by Arseny Tolmachev on 2017/02/27.
//

#include "lattice_types.h"

namespace jumanpp {
namespace core {
namespace analysis {

Lattice::Lattice(util::memory::ManagedAllocatorCore *alloc,
                 const LatticeConfig &lc)
    : boundaries{alloc}, lconf{lc}, alloc{alloc} {}

Status Lattice::makeBoundary(const LatticeBoundaryConfig &lbc,
                             LatticeBoundary **ptr) {
  auto bnd = alloc->make<LatticeBoundary>(alloc, lconf, lbc);
  JPP_RETURN_IF_ERROR(bnd->initialize());
  this->boundaries.push_back(bnd);
  *ptr = bnd;
  return Status::Ok();
}

void Lattice::reset() {
  boundaries.clear();
  boundaries.shrink_to_fit();
}

void Lattice::hintSize(u32 size) { boundaries.reserve(size); }

LatticeBoundary::LatticeBoundary(util::memory::ManagedAllocatorCore *alloc,
                                 const LatticeConfig &lc,
                                 const LatticeBoundaryConfig &lbc)
    : cfg_{lbc},
      left_{alloc, lc, cfg_},
      right_{alloc, lc, cfg_},
      scores_{alloc, lc, cfg_},
      currentEnding_{0} {}

Status LatticeBoundary::initialize() {
  JPP_RETURN_IF_ERROR(left_.initialize());
  JPP_RETURN_IF_ERROR(right_.initialize());
  return Status::Ok();
}

void LatticeBoundary::addEnd(LatticeNodePtr nodePtr) {
  left_.endingNodes.data().at(currentEnding_) = nodePtr;
  ++currentEnding_;
}

LatticeBoundaryScores::LatticeBoundaryScores(
    util::memory::ManagedAllocatorCore *alloc, const LatticeConfig &lc,
    const LatticeBoundaryConfig &lbc)
    : scoresPerItem_{lc.beamSize * lbc.endNodes * lc.scoreCnt},
      numLeft_{lbc.endNodes},
      numBeam_{lc.beamSize},
      numScorers_{lc.scoreCnt} {
  u32 num = lbc.beginNodes;
  // do allocation like this because there can be many elements
  // a single allocation of leftSize * rightSize * beamSize * numScorers can be
  // very large
  auto buffers = alloc->allocateBuf<Score *>(num);
  for (int i = 0; i < num; ++i) {
    buffers[i] = alloc->allocateArray<Score>(scoresPerItem_, 16);
  }
  scores_ = buffers;
}

void LatticeBoundaryScores::importBeamScore(i32 left, i32 scorer, i32 beam,
                                            util::ArraySlice<Score> scores) {
  auto num = scores.size();
  for (int i = 0; i < num; ++i) {
    auto node = nodeScores(i);
    auto svec = node.beamLeft(beam, left);
    svec.at(scorer) = scores.at(i);
  }
}

LatticeRightBoundary::LatticeRightBoundary(
    util::memory::ManagedAllocatorCore *alloc, const LatticeConfig &lc,
    const LatticeBoundaryConfig &lbc)
    : StructOfArrays(alloc, lbc.beginNodes),
      nodeInfo_{this, 1},
      entryDataStorage{this, lc.entrySize},
      primitiveFeatures{this, lc.numPrimitiveFeatures},
      featurePatterns{this, lc.numFeaturePatterns},
      beam{this, lc.beamSize} {}

LatticeLeftBoundary::LatticeLeftBoundary(
    util::memory::ManagedAllocatorCore *alloc, const LatticeConfig &lc,
    const LatticeBoundaryConfig &lbc)
    : StructOfArrays(alloc, lbc.endNodes), endingNodes{this, 1} {}

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp
