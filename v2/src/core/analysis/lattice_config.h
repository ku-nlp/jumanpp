//
// Created by Arseny Tolmachev on 2017/03/02.
//

#ifndef JUMANPP_LATTICE_CONFIG_H
#define JUMANPP_LATTICE_CONFIG_H

#include "util/types.hpp"

namespace jumanpp {
namespace core {
namespace analysis {

class LatticePlugin;

using LatticePosition = u16;
using Score = float;

struct alignas(alignof(u64)) ConnectionPtr {
  u16 boundary;
  u16 left;
  u16 right;
};

struct alignas(alignof(u64)) LatticeNodePtr {
  u16 boundary;
  u16 position;
};

struct LatticeConfig {
  u32 entrySize;
  u32 numPrimitiveFeatures;
  u32 numFeaturePatterns;
  u32 numFinalFeatures;
  u32 beamSize;
  u32 scoreCnt;
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

class Lattice;

}  // analysis
}  // core
}  // jumanpp

#endif  // JUMANPP_LATTICE_CONFIG_H
