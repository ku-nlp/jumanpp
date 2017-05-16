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
    : config{lbc},
      left{alloc, lc, config},
      right{alloc, lc, config},
      connections{alloc, lc, config},
      currentEnding_{0} {}

Status LatticeBoundary::initialize() {
  JPP_RETURN_IF_ERROR(left.initialize());
  JPP_RETURN_IF_ERROR(right.initialize());
  JPP_RETURN_IF_ERROR(connections.initialize());
  return Status::Ok();
}

Status LatticeBoundary::newConnection(LatticeBoundaryConnection **result) {
  *result = &connections.makeOne();
  return Status::Ok();
}

void LatticeBoundary::addEnd(LatticeNodePtr nodePtr) {
  left.endingNodes.data().at(currentEnding_) = nodePtr;
  ++currentEnding_;
}

LatticeBoundaryConnection::LatticeBoundaryConnection(
    util::memory::ManagedAllocatorCore *alloc, const LatticeConfig &lc,
    const LatticeBoundaryConfig &lbc)
    : StructOfArraysFactory(alloc, lbc.beginNodes * lc.beamSize, lbc.endNodes),
      lconf{lc},
      lbconf{lbc},
      scores{this, lc.scoreCnt} {}

LatticeBoundaryConnection::LatticeBoundaryConnection(
    const LatticeBoundaryConnection &o)
    : StructOfArraysFactory(o),
      lconf{o.lconf},
      lbconf{o.lbconf},
      scores{this, o.scores.requiredSize()} {}

void LatticeBoundaryConnection::importBeamScore(
    i32 scorer, i32 beam, util::ArraySlice<Score> scores) {
  auto data = entryScores(beam);
  for (int i = 0; i < data.numRows(); ++i) {
    data.row(i).at(scorer) = scores.at(i);
  }
}

util::Sliceable<Score> LatticeBoundaryConnection::entryScores(i32 beam) {
  auto beamDataSize = lconf.scoreCnt * lbconf.beginNodes;
  auto beamOffset = beam * beamDataSize;
  util::MutableArraySlice<Score> slice{scores.data(), beamOffset, beamDataSize};
  return util::Sliceable<Score>{slice, lconf.scoreCnt, lbconf.beginNodes};
}

LatticeRightBoundary::LatticeRightBoundary(
    util::memory::ManagedAllocatorCore *alloc, const LatticeConfig &lc,
    const LatticeBoundaryConfig &lbc)
    : StructOfArrays(alloc, lbc.beginNodes),
      entryPtrs{this, 1},
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
