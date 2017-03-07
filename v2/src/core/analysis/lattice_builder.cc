//
// Created by Arseny Tolmachev on 2017/02/23.
//

#include "lattice_builder.h"

namespace jumanpp {
namespace core {
namespace analysis {

Status LatticeBuilder::prepare() {
  if (maxBoundaries_ == 0) {
    return Status::Ok();
  }

  boundaries_.resize(maxBoundaries_);

  boundaries_[0].endCount = 1;                     // BOS
  boundaries_[maxBoundaries_ - 1].startCount = 1;  // EOS

  for (auto &seed : seeds_) {
    boundaries_[seed.codepointStart].startCount++;
    boundaries_[seed.codepointEnd].endCount++;
  }

  for (int i = 1; i < boundaries_.size(); ++i) {
    boundaries_[i].number = static_cast<LatticePosition>(i);
    boundaries_[i].firstNodeOffset =
        boundaries_[i - 1].firstNodeOffset + boundaries_[i - 1].startCount;
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
                                               LatticeBoundary **result,
                                               i32 boundaryIdx) {
  JPP_DCHECK_EQ(boundaryIdx,
                lattice->createdBoundaryCount() - 2);  // substract 2 BOS
  auto &bndInfo = boundaries_[boundaryIdx];
  LatticeBoundaryConfig lbc{boundaryIdx + 2u, (u32)bndInfo.endCount,
                            (u32)bndInfo.startCount};
  LatticeBoundary *boundary;
  JPP_RETURN_IF_ERROR(lattice->makeBoundary(lbc, &boundary));
  util::ArraySlice<LatticeNodeSeed> seeds{seeds_, bndInfo.firstNodeOffset,
                                          (u32)bndInfo.startCount};
  auto entryData = boundary->starts()->entryPtrData();
  for (int i = 0; i < entryData.size(); ++i) {
    entryData[i] = seeds[i].entryPtr;
  }

  connectible[boundaryIdx] = lbc.beginNodes != 0 && lbc.endNodes != 0;

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

Status LatticeBuilder::makeEos(LatticeConstructionContext *ctx,
                               Lattice *lattice) {
  LatticeBoundary *lb;
  LatticeBoundaryConfig eos{maxBoundaries_ + 2u,
                            (u32)boundaries_[maxBoundaries_ - 1].endCount, 1u};
  JPP_RETURN_IF_ERROR(lattice->makeBoundary(eos, &lb));
  ctx->addEos(lb);
  return Status::Ok();
}

Status LatticeBuilder::fillEnds(Lattice *l) {
  // connect BOS nodes
  l->boundary(1)->addEnd(LatticeNodePtr{0, 0});
  l->boundary(2)->addEnd(LatticeNodePtr{1, 0});

  for (int i = 0; i < seeds_.size(); ++i) {
    auto &seed = seeds_[i];
    u32 idx = seed.codepointEnd + 2u;
    auto bnd = l->boundary(idx);
    LatticeNodePtr nodePtr{(u16)idx, (u16)seed.codepointStart};
    bnd->addEnd(nodePtr);
  }

  return Status::Ok();
}

void LatticeConstructionContext::addBos(LatticeBoundary *lb) {
  JPP_DCHECK_EQ(lb->localNodeCount(), 1);
  lb->starts()->entryPtrData()[0] = EntryPtr::BOS();
  auto features = lb->starts()->entryData();
  util::fill(features, EntryPtr::BOS().rawValue());
}

void LatticeConstructionContext::addEos(LatticeBoundary *lb) {
  JPP_DCHECK_EQ(lb->localNodeCount(), 1);
  lb->starts()->entryPtrData()[0] = EntryPtr::EOS();
  auto features = lb->starts()->entryData();
  util::fill(features, EntryPtr::EOS().rawValue());
}
}  // analysis
}  // core
}  // jumanpp