//
// Created by Arseny Tolmachev on 2017/02/23.
//

#ifndef JUMANPP_LATTICE_BUILDER_H
#define JUMANPP_LATTICE_BUILDER_H

#include <vector>
#include "lattice_types.h"
#include "util/status.hpp"
#include "util/types.hpp"

namespace jumanpp {
namespace core {
namespace analysis {

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

  u16 endCount;

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

class LatticeConstructionContext {};

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

  util::ArraySlice<LatticeNodeSeed> seeds() const {
    return {seeds_.data(), seeds_.size()};
  }

  Status prepare();

  Status constructSingleBoundary(LatticeConstructionContext* context,
                                 Lattice* lattice) {
    auto boundaryIdx = (u32)lattice->createdBoundaryCount();
    LatticeBoundaryConfig lbc{boundaryIdx,
                              (u32)boundaries_[boundaryIdx].startCount,
                              (u32)boundaries_[boundaryIdx].endCount};
    LatticeBoundary* boundary;
    JPP_RETURN_IF_ERROR(lattice->makeBoundary(lbc, &boundary));
    // fill boundary primitive features
    //
    return Status::Ok();
  }
};

}  // analysis
}  // core
}  // jumanpp

#endif  // JUMANPP_LATTICE_CONSTRUCTION_H
