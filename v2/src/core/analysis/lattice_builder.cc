//
// Created by Arseny Tolmachev on 2017/02/23.
//

#include "lattice_builder.h"

namespace jumanpp {
namespace core {
namespace analysis {

Status LatticeBuilder::prepare() {

  LatticePosition curStart = 0;
  u16 nodeCount = 0;
  u32 nodeOffset = 0;

  auto prepareNodes = [&](const LatticeNodeSeed &seed) {
    while (curStart != seed.codepointStart) {
      boundaries_.emplace_back(curStart, nodeCount, nodeOffset);
      nodeOffset += nodeCount;
      nodeCount = 0;
      curStart += 1;
    }
  };

  for (auto &seed : seeds_) {
    if (seed.codepointStart != curStart) {
      prepareNodes(seed);
    }
    nodeCount += 1;
  }
  LatticePosition eosPos = maxBoundaries_ + 1;
  LatticeNodeSeed eos{EntryPtr::BOS(), eosPos, eosPos};
  prepareNodes(eos);

  if (boundaries_.front().startCount == 0) {
    return Status::InvalidParameter()
           << "could not build lattice, BOS was not connected to anything";
  }
  return Status::Ok();
}

bool LatticeBuilder::checkConnectability() {
  sortSeeds();
  connectible.clear();
  connectible.resize(maxBoundaries_);
  connectible[0] = true;
  for (auto& s: seeds_) {
    if (connectible[s.codepointStart]) {
      connectible[s.codepointEnd] = true;
    }
  }
  return connectible.back();
}

void LatticeBuilder::sortSeeds() {
  std::sort(seeds_.begin(), seeds_.end(),
            [](const LatticeNodeSeed &l, const LatticeNodeSeed &r) -> bool {
              return l.codepointStart < r.codepointStart;
            });
}

void LatticeBuilder::reset(LatticePosition maxCodepoints) {
  ++maxCodepoints;
  LatticePosition  maxBoundaries = maxCodepoints;
  seeds_.clear();
  boundaries_.clear();
  boundaries_.reserve(
      static_cast<std::make_unsigned<LatticePosition>::type>(maxBoundaries));
  maxBoundaries_ = maxBoundaries;
}

Status LatticeBuilder::constructSingleBoundary(LatticeConstructionContext *context, Lattice *lattice) {
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

Status LatticeBuilder::makeBos(LatticeConstructionContext *ctx, Lattice *lattice) {
  LatticeBoundary *lb;
  LatticeBoundaryConfig bos1 {
      0, 0, 1
  };
  JPP_RETURN_IF_ERROR(lattice->makeBoundary(bos1, &lb));
  ctx->addBos(lb);
  LatticeBoundaryConfig bos2 {
      1, 1, 1
  };
  JPP_RETURN_IF_ERROR(lattice->makeBoundary(bos2, &lb));
  ctx->addBos(lb);

  return Status::Ok();
}

}  // analysis
}  // core
}  // jumanpp