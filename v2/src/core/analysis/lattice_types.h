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

class LatticeBoundaryConnection final
    : public util::memory::StructOfArraysFactory<LatticeBoundaryConnection> {
  const LatticeConfig& lconf;
  const LatticeBoundaryConfig& lbconf;
  util::memory::SizedArrayField<Score> scores;

  friend class LatticeBoundary;

 public:
  LatticeBoundaryConnection(util::memory::ManagedAllocatorCore* alloc,
                            const LatticeConfig& lc,
                            const LatticeBoundaryConfig& lbc);

  LatticeBoundaryConnection(const LatticeBoundaryConnection& o);

  LatticeBoundaryConnection* element(i32 idx) { return child(idx); }

  void importBeamScore(i32 scorer, i32 beam, util::ArraySlice<Score> scores);
  util::Sliceable<Score> entryScores(i32 beam);
};

class LatticeBoundary {
  LatticeBoundaryConfig config;
  LatticeLeftBoundary left;
  LatticeRightBoundary right;
  LatticeBoundaryConnection connections;
  u32 currentEnding_;

  Status initialize();

 public:
  LatticeBoundary(util::memory::ManagedAllocatorCore* alloc,
                  const LatticeConfig& lc, const LatticeBoundaryConfig& lbc);

  EntryPtr entry(u32 position) const {
    return right.nodeInfo_.data().at(position).entryPtr();
  }

  bool endingsFilled() const {
    return left.endingNodes.data().size() == currentEnding_;
  }

  bool isActive() const {
    return config.beginNodes != 0 && config.endNodes != 0;
  }

  LatticeLeftBoundary* ends() { return &left; }
  const LatticeLeftBoundary* ends() const { return &left; }
  LatticeRightBoundary* starts() { return &right; }
  const LatticeRightBoundary* starts() const { return &right; }
  u32 localNodeCount() const { return config.beginNodes; }

  friend class Lattice;

  void addEnd(LatticeNodePtr nodePtr);
  Status newConnection(LatticeBoundaryConnection** result);
  LatticeBoundaryConnection* connection(i32 index) {
    return connections.element(index);
  }
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
