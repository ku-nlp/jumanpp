//
// Created by Arseny Tolmachev on 2017/03/03.
//

#ifndef JUMANPP_ANALYZER_IMPL_H
#define JUMANPP_ANALYZER_IMPL_H

#include "core/analysis/analysis_input.h"
#include "core/analysis/analyzer.h"
#include "core/analysis/extra_nodes.h"
#include "core/analysis/lattice_builder.h"
#include "core/analysis/lattice_types.h"
#include "core/analysis/score_processor.h"

namespace jumanpp {
namespace core {
namespace analysis {

class AnalyzerImpl {
 protected:
  AnalyzerConfig cfg_;
  const CoreHolder* core_;
  util::memory::Manager memMgr_;
  std::unique_ptr<util::memory::ManagedAllocatorCore> alloc_;
  AnalysisInput input_;
  LatticeConfig latticeConfig_;
  Lattice lattice_;
  LatticeBuilder latticeBldr_;
  ExtraNodesContext xtra_;
  OutputManager outputManager_;
  ScoreProcessor* sproc_;
  std::vector<std::unique_ptr<ScoreComputer>> scorers_;
  LatticeCompactor compactor_;
  NgramStats ngramStats_;

 public:
  AnalyzerImpl(const AnalyzerImpl&) = delete;
  AnalyzerImpl(AnalyzerImpl&&) = delete;
  AnalyzerImpl(const CoreHolder* core, const ScoringConfig& sconf,
               const AnalyzerConfig& cfg);

  const OutputManager& output() const { return outputManager_; }

  Status initScorers(const ScorerDef& cfg);

  void reset() {
    lattice_.reset();
    xtra_.reset();
    memMgr_.reset();
    alloc_->reset();
    sproc_ = nullptr;
  }

  // This set of functions is internal
  // They are called from prepareNodeSeeds
  Status makeNodeSeedsFromDic();
  Status makeUnkNodes1();
  bool checkLatticeConnectivity();
  Status makeUnkNodes2();

  /**
   * This function calls reset() internally
   * and can not be used in training.
   */
  Status resetForInput(StringPiece input);
  Status setNewInput(StringPiece input);
  Status prepareNodeSeeds();
  Status buildLattice();
  Status bootstrapAnalysis();
  Status computeScores(const ScorerDef* sconf);

  Lattice* lattice() { return &lattice_; }
  const Lattice* lattice() const { return &lattice_; }
  LatticeBuilder* latticeBldr() { return &latticeBldr_; }
  ExtraNodesContext* extraNodesContext() { return &xtra_; }
  const dic::DictionaryHolder& dic() const { return core_->dic(); }
  const CoreHolder& core() const { return *core_; }

  u64 allocatedMemory() const {
    return memMgr_.used() + latticeBldr_.usedMemory() + sizeof(AnalyzerImpl);
  }

  u64 usedMemory() const {
    auto allocated = allocatedMemory();
    auto remaining = alloc_->remaining();
    return allocated - remaining;
  }

  util::memory::ManagedAllocatorCore* alloc() const { return alloc_.get(); }
  const NgramStats& ngramStats() const { return ngramStats_; }
};

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_ANALYZER_IMPL_H
