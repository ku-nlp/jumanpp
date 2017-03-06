//
// Created by Arseny Tolmachev on 2017/02/23.
//

#ifndef JUMANPP_LATTICE_BUILDER_H
#define JUMANPP_LATTICE_BUILDER_H

#include <util/stl_util.h>
#include <vector>
#include "core/core_types.h"
#include "extra_nodes.h"
#include "lattice_types.h"
#include "util/status.hpp"
#include "util/types.hpp"

namespace jumanpp {
namespace core {
namespace analysis {

struct LatticeNodeSeed {
  EntryPtr entryPtr;
  LatticePosition codepointStart;
  LatticePosition codepointEnd;

  LatticeNodeSeed(EntryPtr entryPtr, LatticePosition codepointStart,
                  LatticePosition codepointEnd)
      : entryPtr(entryPtr),
        codepointStart(codepointStart),
        codepointEnd(codepointEnd) {}
};

struct BoundaryInfo {
  /**
   * Index of first input codepoint for this boundary
   */
  LatticePosition number;

  /**
   * Number of nodes starting from this boundary
   */
  u16 startCount;

  u16 endCount;

  /**
   * If you put all nodes lattice nodes in a single array,
   * ordered by index of first codepoint, then this will
   * be the index of the first node with starting codepoint == \refitem number
   */
  u32 firstNodeOffset;
};

class LatticeConstructionContext {
 public:
  Status addBos(LatticeBoundary* lb);
};

class LatticeBuilder {
  std::vector<LatticeNodeSeed> seeds_;
  std::vector<BoundaryInfo> boundaries_;
  std::vector<bool> connectible;
  LatticePosition maxBoundaries_;

 public:
  inline void appendSeed(EntryPtr entryPtr, LatticePosition start,
                         LatticePosition end) {
    seeds_.emplace_back(entryPtr, start, end);
  }

  void sortSeeds();
  bool checkConnectability();
  void reset(LatticePosition maxCodepoints);

  util::ArraySlice<LatticeNodeSeed> seeds() const {
    return {seeds_.data(), seeds_.size()};
  }

  Status prepare();
  Status constructSingleBoundary(Lattice* lattice, LatticeBoundary** result,
                                 i32 numBoundary);
  bool isAccessible(i32 boundary) const { return connectible[boundary]; }

  Status makeBos(LatticeConstructionContext* ctx, Lattice* lattice);
};

}  // analysis
}  // core
}  // jumanpp

#endif  // JUMANPP_LATTICE_CONSTRUCTION_H
