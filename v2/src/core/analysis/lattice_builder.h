//
// Created by Arseny Tolmachev on 2017/02/23.
//

#ifndef JUMANPP_LATTICE_BUILDER_H
#define JUMANPP_LATTICE_BUILDER_H

#include <vector>
#include "util/status.hpp"
#include "util/types.hpp"

namespace jumanpp {
namespace core {
namespace analysis {

using LatticePosition = u16;

struct LatticeNodeSeed {
  i32 entryPtr;
  LatticePosition codepointStart;
  LatticePosition codepointEnd;

  LatticeNodeSeed(i32 entryPtr, LatticePosition codepointStart,
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

  /**
   * If you put all nodes lattice nodes in a single array,
   * ordered by index of first codepoint, then this will
   * be the index of the first node with starting codepoint == \refitem number
   */
  u32 firstNodeOffset;

  BoundaryInfo(u16 number, u16 startCount, u32 firstNodeOffset)
      : number(number),
        startCount(startCount),
        firstNodeOffset(firstNodeOffset) {}
};

class LatticeBuilder {
  std::vector<LatticeNodeSeed> seeds_;
  std::vector<BoundaryInfo> boundaries_;
  LatticePosition maxBoundaries_;

 public:
  inline void appendSeed(i32 entryPtr, LatticePosition start,
                         LatticePosition end) {
    seeds_.emplace_back(entryPtr, start, end);
  }

  void reset(LatticePosition maxBoundaries) {
    seeds_.clear();
    boundaries_.clear();
    boundaries_.reserve(
        static_cast<std::make_unsigned<LatticePosition>::type>(maxBoundaries));
    maxBoundaries_ = maxBoundaries;
  }

  const std::vector<LatticeNodeSeed>& seeds() { return seeds_; }

  Status prepare();
};

}  // analysis
}  // core
}  // jumanpp

#endif  // JUMANPP_LATTICE_CONSTRUCTION_H
