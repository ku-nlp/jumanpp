//
// Created by Arseny Tolmachev on 2017/02/23.
//

#include "lattice_builder.h"
#include "util/array_slice_util.h"
#include "util/hashing.h"

#include "util/debug_output.h"
#include "util/logging.hpp"

namespace jumanpp {
namespace core {
namespace analysis {

Status LatticeBuilder::prepare() {
  if (maxBoundaries_ == 0) {
    return Status::Ok();
  }

  boundaries_.clear();
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
  std::stable_sort(
      seeds_.begin(), seeds_.end(),
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
  auto entryData = boundary->starts()->nodeInfo();
  for (int i = 0; i < entryData.size(); ++i) {
    auto &seed = seeds[i];
    entryData[i] =
        NodeInfo{seed.entryPtr, seed.codepointStart, seed.codepointEnd};
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
    if (seed.codepointStart == seed.codepointEnd) {
      continue;
    }
    u32 idx = seed.codepointEnd + 2u;
    auto bnd = l->boundary(idx);
    u16 bndPtr = (u16)(seed.codepointStart + 2);
    auto binfo = boundaries_[seed.codepointStart];
    i32 offset = i - binfo.firstNodeOffset;
    JPP_DCHECK_IN(offset, 0, binfo.startCount);
    u16 endOffset = (u16)offset;
    LatticeNodePtr nodePtr{bndPtr, endOffset};
    JPP_DCHECK_EQ(
        l->boundary(bndPtr)->starts()->nodeInfo().at(offset).entryPtr(),
        seed.entryPtr);
    bnd->addEnd(nodePtr);
  }

  return Status::Ok();
}

void LatticeBuilder::compactBoundary(i32 boundary,
                                     LatticeCompactor *compactor) {
  auto &bndInfo = boundaries_[boundary];
  if (bndInfo.startCount < 2) {
    return;
  }
  util::MutableArraySlice<LatticeNodeSeed> seeds{
      &seeds_, bndInfo.firstNodeOffset, (u32)bndInfo.startCount};
  compactor->computeHashes(seeds);
  if (compactor->compact(&seeds)) {
    // mark evacuated seeds as erased
    auto start = static_cast<i32>(seeds.size()) - compactor->numDeleted();
    for (int idx = start; idx < seeds.size(); ++idx) {
      auto &s = seeds.at(idx);
      boundaries_[s.codepointEnd].endCount -= 1;
      s.codepointEnd = s.codepointStart;
    }
    bndInfo.startCount = static_cast<u16>(start);
  }
}

const BoundaryInfo &LatticeBuilder::infoAt(i32 boundary) const {
  JPP_DCHECK_IN(boundary, 0, boundaries_.size());
  return boundaries_[boundary];
}

void LatticeConstructionContext::addBos(LatticeBoundary *lb) {
  JPP_DCHECK_EQ(lb->localNodeCount(), 1);
  lb->starts()->nodeInfo()[0] = NodeInfo{EntryPtr::BOS(), 0, 0};
  auto features = lb->starts()->patternFeatureData();
  util::fill(features, EntryPtr::BOS().rawValue());
}

void LatticeConstructionContext::addEos(LatticeBoundary *lb) {
  JPP_DCHECK_EQ(lb->localNodeCount(), 1);
  lb->starts()->nodeInfo()[0] = NodeInfo{EntryPtr::EOS(), 0, 0};
  auto features = lb->starts()->patternFeatureData();
  util::fill(features, EntryPtr::EOS().rawValue());
}

void LatticeCompactor::computeHashes(util::ArraySlice<LatticeNodeSeed> seeds) {
  entries.clear();
  entries.resize(seeds.size() * dicEntries.numFeatures());
  hashes.clear();
  hashes.resize(seeds.size());

  util::Sliceable<i32> entryData{&entries, (u32)dicEntries.numFeatures(),
                                 seeds.size()};
  for (int i = 0; i < seeds.size(); ++i) {
    auto &s = seeds[i];
    auto result = entryData.row(i);
    auto ptr = s.entryPtr;
    if (ptr.isSpecial()) {
      util::copy_buffer(xtra->nodeContent(xtra->node(ptr)), result);
    } else {
      dicEntries.entryAtPtr(ptr.dicPtr()).fill(result, result.size());
    }

    hashes[i] = util::hashing::hashIndexedSeq(0x23fa23a12, result, features);
  }
}

class ValueChecker {
  util::ArraySlice<i32> indices;

 public:
  ValueChecker(const util::ArraySlice<i32> &indices) : indices(indices) {}

  bool valuesEqual(const util::Sliceable<i32> &data, i32 e1, i32 e2) const {
    auto row1 = data.row(e1);
    auto row2 = data.row(e2);

    for (auto &idx : indices) {
      if (row1.at(idx) != row2.at(idx)) {
        return false;
      }
    }
    return true;
  }
};

std::ostream &operator<<(std::ostream &os,
                         const core::analysis::LatticeNodeSeed &seed) {
  os << '{' << seed.codepointStart << "->" << seed.codepointEnd << ":"
     << seed.entryPtr.rawValue() << '}';
  return os;
}

std::ostream &operator<<(std::ostream &os, const core::EntryPtr &eptr) {
  os << '@' << eptr.rawValue();
  return os;
}

bool LatticeCompactor::compact(
    util::MutableArraySlice<LatticeNodeSeed> *seeds) {
  processed.clear_no_resize();
  auto numElems = hashes.size();
  JPP_DCHECK_EQ(seeds->size(), numElems);
  util::Sliceable<i32> entryData{&entries, (u32)dicEntries.numFeatures(),
                                 numElems};
  ValueChecker valueChecker{features};
  for (int i = 0; i < numElems; ++i) {
    if (processed.count(i) != 0) {
      continue;
    }
    group.clear();
    for (int j = i + 1; j < numElems; ++j) {
      if (processed.count(j) != 0) {
        continue;
      }
      if (hashes[i] == hashes[j] && valueChecker.valuesEqual(entryData, i, j)) {
        //        LOG_TRACE() << "COMPACT SAME=" << VOut(entryData.row(j))
        //                    << " SEED=" << seeds->at(j) << " IDX=" << j;
        group.push_back(j);
        processed.insert(j);
      }
    }

    //    LOG_TRACE() << "COMPACT BASE=" << VOut(entryData.row(i))
    //                << " SEED=" << seeds->at(i) << " COMPACT=" <<
    //                !group.empty()
    //                << " IDX=" << i;

    if (!group.empty()) {
      auto node = xtra->makeAlias();
      auto data = xtra->nodeContent(node);
      util::copy_buffer(entryData.row(i), data);
      auto buffer = xtra->aliasBuffer(node, group.size() + 1);
      buffer[0] = seeds->at(i).entryPtr.rawValue();
      for (int j = 0; j < group.size(); ++j) {
        buffer[j + 1] = seeds->at(group[j]).entryPtr.rawValue();
      }
      seeds->at(i).entryPtr = node->ptr();
    }
  }

  // there weren't any combinable nodes
  if (processed.empty()) {
    return false;
  }

  group.clear();
  util::copy_insert(processed, group);
  util::sort(group);
  //  LOG_TRACE() << "PRECOMPACT" << VOut(*seeds) << " REMOVE=" << VOut(group)
  //              << " SIZE=" << seeds->size();
  util::compact(*seeds, group);
  //  LOG_TRACE() << "POSTCOMPACT" << VOut(*seeds);

  return true;
}

LatticeCompactor::LatticeCompactor(const dic::DictionaryEntries &dicEntries)
    : dicEntries(dicEntries) {}

Status LatticeCompactor::initialize(ExtraNodesContext *ctx,
                                    const RuntimeInfo &spec) {
  xtra = ctx;

  features.clear();
  for (auto &f : spec.features.primitive) {
    if (f.kind == spec::PrimitiveFeatureKind::Copy) {
      features.push_back(f.references[0]);
    }
  }

  util::sort(features);

  return Status::Ok();
}

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp