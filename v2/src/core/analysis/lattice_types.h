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

class LatticePlugin;

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
  util::memory::SizedArrayField<EntryPtr> entryPtrs;
  util::memory::SizedArrayField<i32, 64> entryDataStorage;
  util::memory::SizedArrayField<u64, 64> primitiveFeatures;
  util::memory::SizedArrayField<u64, 64> featurePatterns;
  util::memory::SizedArrayField<ConnectionBeamElement, 64> beam;

  LatticePlugin* localPlugin = nullptr;

  friend class LatticeBoundary;

 public:
  LatticeRightBoundary(util::memory::ManagedAllocatorCore* alloc,
                       const LatticeConfig& lc,
                       const LatticeBoundaryConfig& lbc);

  template <typename Plugin>
  Plugin* plugin() {
    return dynamic_cast<Plugin*>(localPlugin);
  }

  util::Sliceable<EntryPtr> entryPtrData() { return entryPtrs; }
  util::Sliceable<i32> entryData() { return entryDataStorage; }
  util::Sliceable<u64> primitiveFeatureData() { return primitiveFeatures; }
  util::Sliceable<u64> patternFeatureData() { return featurePatterns; }
  util::Sliceable<ConnectionBeamElement> beamData() { return beam; }
};

class LatticeBoundaryConnection final
    : public util::memory::StructOfArraysFactory<LatticeBoundaryConnection> {
  const LatticeConfig& lconf;
  const LatticeBoundaryConfig& lbconf;
  util::memory::SizedArrayField<Score> scores;
  LatticePlugin* localPlugin = nullptr;

  friend class LatticeBoundary;

 public:
  LatticeBoundaryConnection(util::memory::ManagedAllocatorCore* alloc,
                            const LatticeConfig& lc,
                            const LatticeBoundaryConfig& lbc);

  LatticeBoundaryConnection(const LatticeBoundaryConnection& o);

  LatticeBoundaryConnection* element(i32 idx) { return child(idx); }

  void importBeamScore(i32 scorer, i32 beam, util::ArraySlice<Score> scores);
  util::Sliceable<Score> entryScores(i32 beam);

  template <typename Plugin>
  Plugin* plugin() {
    return dynamic_cast<Plugin*>(localPlugin);
  }
};

class LatticeBoundary {
  LatticeBoundaryConfig config;
  LatticeLeftBoundary left;
  LatticeRightBoundary right;
  LatticeBoundaryConnection connections;
  u32 currentEnding_;

  Status initialize();
  void installPlugin(LatticePlugin* plugin);

 public:
  LatticeBoundary(util::memory::ManagedAllocatorCore* alloc,
                  const LatticeConfig& lc, const LatticeBoundaryConfig& lbc);

  EntryPtr entry(u32 position) const {
    return right.entryPtrs.data().at(position);
  }

  bool endingsFilled() const {
    return left.endingNodes.data().size() == currentEnding_;
  }

  LatticeLeftBoundary* ends() { return &left; }
  LatticeRightBoundary* starts() { return &right; }
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
  LatticePlugin* plugin = nullptr;

 public:
  Lattice(const Lattice&) = delete;
  Lattice(util::memory::ManagedAllocatorCore* alloc, const LatticeConfig& lc);
  u32 createdBoundaryCount() const { return (u32)boundaries.size(); }
  Status makeBoundary(const LatticeBoundaryConfig& lbc, LatticeBoundary** ptr);
  LatticeBoundary* boundary(u32 idx) { return boundaries.at(idx); }
  const LatticeBoundary* boundary(u32 idx) const { return boundaries.at(idx); }
  void hintSize(u32 size);
  void installPlugin(LatticePlugin* plugin);
  void reset();
  const LatticeConfig& config() { return lconf; }
};

}  // analysis
}  // core
}  // jumanpp

#endif  // JUMANPP_LATTICE_TYPES_H
