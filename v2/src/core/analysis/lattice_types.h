//
// Created by Arseny Tolmachev on 2017/02/26.
//

#ifndef JUMANPP_LATTICE_TYPES_H
#define JUMANPP_LATTICE_TYPES_H

#include "util/soa.h"
#include "util/types.hpp"
#include "lattice_builder.h"

namespace jumanpp {
namespace core {
namespace analysis {

using Score = float;

struct ConnectionPtr {
  u16 boundary;
  u16 left;
  u16 right;
  u16 beam;
};

struct LatticeNodePtr {
  u16 boundary;
  u16 position;
};

struct LatticeConfig {
  u32 numBoundaries;
  u32 numPrimitiveFeatures;
  u32 numFeaturePatterns;
  u32 numFinalFeatures;
  u32 beamSize;
};

struct LatticeBoundaryConfig {
  u32 boundary;
  u32 endNodes;
  u32 beginNodes;
};

struct ConnectionBeamElement {
  ConnectionPtr ptr;
  Score totalScore;
};

class LatticeLeftBoundary final : public util::memory::StructOfArrays {
  util::memory::SizedArrayField<LatticeNodePtr> endingNodes;
public:
  LatticeLeftBoundary(util::memory::ManagedAllocatorCore* alloc, const LatticeConfig& lc, const LatticeBoundaryConfig& lbc):
      StructOfArrays(alloc, lbc.endNodes),
      endingNodes{this, 1} {}
};

class LatticeRightBoundary final : public util::memory::StructOfArrays {
  util::memory::SizedArrayField<i32> dicPtrs;
  util::memory::SizedArrayField<i32, 64> primitiveFeatures;
  util::memory::SizedArrayField<u64, 64> featurePatterns;
  util::memory::SizedArrayField<ConnectionBeamElement, 64> beam;
public:
  LatticeRightBoundary(util::memory::ManagedAllocatorCore* alloc, const LatticeConfig& lc, const LatticeBoundaryConfig& lbc):
      StructOfArrays(alloc, lbc.beginNodes),
      dicPtrs{this, 1},
      primitiveFeatures{this, lc.numPrimitiveFeatures},
      featurePatterns{this, lc.numFeaturePatterns},
      beam{this, lc.beamSize} {}
};

class LatticeBoundaryConnection final: public util::memory::StructOfArraysFactory<LatticeBoundaryConnection> {
  util::memory::SizedArrayField<u32, 64> features;
  util::memory::SizedArrayField<Score> featureScores;
public:
  LatticeBoundaryConnection(util::memory::ManagedAllocatorCore* alloc, const LatticeConfig& lc, const LatticeBoundaryConfig& lbc):
    StructOfArraysFactory(alloc, lbc.beginNodes * lc.beamSize, lbc.endNodes),
    features{this, lc.numFinalFeatures},
    featureScores{this, 1}{}

  LatticeBoundaryConnection(const LatticeBoundaryConnection& o):
      StructOfArraysFactory(o),
      features{this, o.features.requiredSize()},
      featureScores{this, 1} {}
};

class LatticeBoundary {
  LatticeBoundaryConfig config;
  LatticeLeftBoundary left;
  LatticeRightBoundary right;
  LatticeBoundaryConnection connections;

public:
  LatticeBoundary(util::memory::ManagedAllocatorCore* alloc, const LatticeConfig& lc, const LatticeBoundaryConfig& lbc):
      config{lbc},
      left{alloc, lc, lbc},
      right{alloc, lc, lbc},
      connections{alloc, lc, lbc} {}
};

class Lattice {
  util::memory::ManagedVector<LatticeBoundary> boundaries;
  LatticeConfig lconf;
  util::memory::ManagedAllocatorCore* alloc;

public:
  Lattice(util::memory::ManagedAllocatorCore* alloc, const LatticeConfig& lc): boundaries{alloc}, alloc{alloc}, lconf{lc} {
    boundaries.reserve(lc.numBoundaries);
  }
};

}  // analysis
}  // core
}  // jumanpp

#endif  // JUMANPP_LATTICE_TYPES_H
