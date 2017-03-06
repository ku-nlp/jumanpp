//
// Created by Arseny Tolmachev on 2017/02/23.
//

#include "lattice_builder.h"

namespace jumanpp {
namespace core {
namespace analysis {

Status LatticeBuilder::prepare() {
  boundaries_.resize(maxBoundaries_);

  for (auto &seed : seeds_) {
    boundaries_[seed.codepointStart].startCount++;
    boundaries_[seed.codepointEnd].endCount++;
  }

  for (int i = 1; i < boundaries_.size(); ++i) {
    boundaries_[i].number = static_cast<LatticePosition>(i);
    boundaries_[i].firstNodeOffset =
        boundaries_[i - 1].firstNodeOffset + boundaries_[i].startCount;
  }

  return Status::Ok();
}

bool LatticeBuilder::checkConnectability() {
  sortSeeds();
  connectible.clear();
  connectible.resize(maxBoundaries_);
  connectible[0] = true;
  for (auto &s : seeds_) {
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
  LatticePosition maxBoundaries = maxCodepoints;
  seeds_.clear();
  boundaries_.clear();
  maxBoundaries_ = maxBoundaries;
}

Status LatticeBuilder::constructSingleBoundary(Lattice *lattice,
                                               LatticeBoundary **result) {
  auto boundaryIdx =
      (u32)lattice->createdBoundaryCount() - 2;  // substract 2 BOS
  auto &bndInfo = boundaries_[boundaryIdx];
  LatticeBoundaryConfig lbc{boundaryIdx,
                            (u32)bndInfo.endCount,
                            (u32)bndInfo.startCount};
  LatticeBoundary *boundary;
  JPP_RETURN_IF_ERROR(lattice->makeBoundary(lbc, &boundary));
  util::ArraySlice<LatticeNodeSeed> seeds{seeds_, bndInfo.firstNodeOffset,
                                          (u32)bndInfo.startCount};
  auto entryData = boundary->starts()->entryPtrData();
  for (int i = 0; i < entryData.size(); ++i) {
    entryData[i] = seeds[i].entryPtr;
  }

  *result = boundary;
  return Status::Ok();
}

Status LatticeBuilder::makeBos(LatticeConstructionContext *ctx,
                               Lattice *lattice) {
  LatticeBoundary *lb;
  LatticeBoundaryConfig bos1{0, 0, 1};
  JPP_RETURN_IF_ERROR(lattice->makeBoundary(bos1, &lb));
  ctx->addBos(lb);
  LatticeBoundaryConfig bos2{1, 1, 1};
  JPP_RETURN_IF_ERROR(lattice->makeBoundary(bos2, &lb));
  ctx->addBos(lb);

  return Status::Ok();
}

Status LatticeConstructionContext::addBos(LatticeBoundary *lb) {
  JPP_DCHECK_EQ(lb->localNodeCount(), 1);
  lb->starts()->entryPtrData()[0] = EntryPtr::BOS();
  auto features = lb->starts()->entryData();
  util::fill(features, EntryPtr::BOS().rawValue());
  return Status::Ok();
}
}  // analysis
}  // core
}  // jumanpp