//
// Created by Arseny Tolmachev on 2017/02/26.
//

#ifndef JUMANPP_LATTICE_TYPES_H
#define JUMANPP_LATTICE_TYPES_H

#include "core/analysis/lattice_config.h"
#include "core/core_types.h"
#include "util/soa.h"
#include "util/types.hpp"

namespace jumanpp {
namespace core {
namespace analysis {

class LatticeLeftBoundary final : public util::memory::StructOfArrays {
  util::memory::SizedArrayField<LatticeNodePtr> endingNodes;

  friend class LatticeBoundary;

 public:
  LatticeLeftBoundary(util::memory::ManagedAllocatorCore* alloc,
                      const LatticeConfig& lc,
                      const LatticeBoundaryConfig& lbc);

  util::ArraySlice<LatticeNodePtr> nodePtrs() const {
    return endingNodes.data();
  }
};

class LatticeRightBoundary final : public util::memory::StructOfArrays {
  util::memory::SizedArrayField<NodeInfo> nodeInfo_;
  util::memory::SizedArrayField<i32, 64> entryDataStorage;
  util::memory::SizedArrayField<u64, 64> primitiveFeatures;
  util::memory::SizedArrayField<u64, 64> featurePatterns;
  util::memory::SizedArrayField<ConnectionBeamElement, 64> beam;

  friend class LatticeBoundary;

 public:
  LatticeRightBoundary(util::memory::ManagedAllocatorCore* alloc,
                       const LatticeConfig& lc,
                       const LatticeBoundaryConfig& lbc);

  util::Sliceable<NodeInfo> nodeInfo() { return nodeInfo_; }
  util::ConstSliceable<NodeInfo> nodeInfo() const { return nodeInfo_; }
  util::Sliceable<i32> entryData() { return entryDataStorage; }
  util::ConstSliceable<i32> entryData() const { return entryDataStorage; }
  util::Sliceable<u64> primitiveFeatureData() { return primitiveFeatures; }
  util::ConstSliceable<u64> primitiveFeatureData() const {
    return primitiveFeatures;
  }
  util::Sliceable<u64> patternFeatureData() { return featurePatterns; }
  util::ConstSliceable<u64> patternFeatureData() const {
    return featurePatterns;
  }
  util::Sliceable<ConnectionBeamElement> beamData() { return beam; }
  util::ConstSliceable<ConnectionBeamElement> beamData() const { return beam; }
};

class NodeScores {
  util::Sliceable<Score> beamScores_;
  u32 numLeft_;
  u32 numScorers_;

 public:
  NodeScores(const util::Sliceable<Score>& beamScores, u32 numLeft,
             u32 numScorers)
      : beamScores_(beamScores), numLeft_(numLeft), numScorers_(numScorers) {}

  util::MutableArraySlice<Score> beamLeft(i32 beam, i32 left) {
    auto data = beamScores_.row(beam);
    JPP_DCHECK_IN(left, 0, numLeft_);
    auto numPerRow = numScorers_;
    u32 start = left * numPerRow;
    util::MutableArraySlice<Score> slice{data, start, numScorers_};
    return slice;
  }

  u32 beam() const { return static_cast<u32>(beamScores_.numRows()); }
  u32 left() const { return numLeft_; }
  u32 numScorers() const { return numScorers_; }
};

class LatticeBoundaryScores final {
  util::ArraySlice<Score*> scores_;
  u32 scoresPerItem_;
  u32 numLeft_;
  u32 numBeam_;
  u32 numScorers_;

  friend class NodeScores;

 public:
  LatticeBoundaryScores(util::memory::ManagedAllocatorCore* alloc,
                        const LatticeConfig& lc,
                        const LatticeBoundaryConfig& lbc);

  NodeScores nodeScores(i32 right) const noexcept {
    auto data = scores_.at(right);
    util::MutableArraySlice<Score> slice{data, scoresPerItem_};
    util::Sliceable<Score> sl{slice, numLeft_ * numScorers_, numBeam_};
    return NodeScores{sl, numLeft_, numScorers_};
  }

  util::ArraySlice<Score> forPtr(const ConnectionPtr& ptr) const {
    return nodeScores(ptr.right).beamLeft(ptr.beam, ptr.left);
  }

  void importBeamScore(i32 left, i32 scorer, i32 beam,
                       util::ArraySlice<Score> scores);
};

class LatticeBoundary {
  LatticeBoundaryConfig cfg_;
  LatticeLeftBoundary left_;
  LatticeRightBoundary right_;
  LatticeBoundaryScores scores_;
  u32 currentEnding_;

  Status initialize();

 public:
  LatticeBoundary(util::memory::ManagedAllocatorCore* alloc,
                  const LatticeConfig& lc, const LatticeBoundaryConfig& lbc);

  EntryPtr entry(u32 position) const {
    return right_.nodeInfo_.data().at(position).entryPtr();
  }

  bool endingsFilled() const {
    return left_.endingNodes.data().size() == currentEnding_;
  }

  bool isActive() const { return cfg_.beginNodes != 0 && cfg_.endNodes != 0; }

  LatticeLeftBoundary* ends() { return &left_; }
  const LatticeLeftBoundary* ends() const { return &left_; }
  LatticeRightBoundary* starts() { return &right_; }
  const LatticeRightBoundary* starts() const { return &right_; }
  u32 localNodeCount() const { return cfg_.beginNodes; }

  LatticeBoundaryScores* scores() { return &scores_; }

  const LatticeBoundaryScores* scores() const { return &scores_; }

  friend class Lattice;

  void addEnd(LatticeNodePtr nodePtr);
};

class Lattice {
  util::memory::ManagedVector<LatticeBoundary*> boundaries;
  LatticeConfig lconf;
  util::memory::ManagedAllocatorCore* alloc;

 public:
  Lattice(const Lattice&) = delete;
  Lattice(util::memory::ManagedAllocatorCore* alloc, const LatticeConfig& lc);
  u32 createdBoundaryCount() const { return (u32)boundaries.size(); }
  Status makeBoundary(const LatticeBoundaryConfig& lbc, LatticeBoundary** ptr);
  LatticeBoundary* boundary(i32 idx) { return boundary(static_cast<u32>(idx)); }
  LatticeBoundary* boundary(u32 idx) {
    JPP_DCHECK_IN(idx, 0, boundaries.size());
    return boundaries[idx];
  }
  const LatticeBoundary* boundary(i32 idx) const {
    return boundary(static_cast<u32>(idx));
  }
  const LatticeBoundary* boundary(u32 idx) const {
    JPP_DCHECK_IN(idx, 0, boundaries.size());
    return boundaries.at(idx);
  }
  void hintSize(u32 size);
  void reset();
  const LatticeConfig& config() { return lconf; }
};

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_LATTICE_TYPES_H
