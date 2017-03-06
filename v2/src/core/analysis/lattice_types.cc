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

void Lattice::reset() { boundaries.clear(); }

LatticeBoundary::LatticeBoundary(util::memory::ManagedAllocatorCore *alloc,
                                 const LatticeConfig &lc,
                                 const LatticeBoundaryConfig &lbc)
    : config{lbc},
      left{alloc, lc, lbc},
      right{alloc, lc, lbc},
      connections{alloc, lc, lbc} {}

LatticeBoundaryConnection::LatticeBoundaryConnection(
    util::memory::ManagedAllocatorCore *alloc, const LatticeConfig &lc,
    const LatticeBoundaryConfig &lbc)
    : StructOfArraysFactory(alloc, lbc.beginNodes * lc.beamSize, lbc.endNodes),
      features{this, lc.numFinalFeatures},
      featureScores{this, 1} {}

LatticeBoundaryConnection::LatticeBoundaryConnection(
    const LatticeBoundaryConnection &o)
    : StructOfArraysFactory(o),
      features{this, o.features.requiredSize()},
      featureScores{this, 1} {}

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
}  // analysis
}  // core
}  // jumanpp
