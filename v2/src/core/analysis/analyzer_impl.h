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

namespace jumanpp {
namespace core {
namespace analysis {

class AnalyzerImpl {
  AnalyzerConfig cfg_;
  const CoreHolder* core_;
  util::memory::Manager memMgr_;
  std::unique_ptr<util::memory::ManagedAllocatorCore> alloc_;
  AnalysisInput input_;
  Lattice lattice_;
  LatticeBuilder latticeBldr_;
  ExtraNodesContext xtra_;
  OutputManager outputManager_;

 public:
  AnalyzerImpl(const AnalyzerImpl&) = delete;
  AnalyzerImpl(AnalyzerImpl&&) = delete;

  AnalyzerImpl(const CoreHolder* core, const AnalyzerConfig& cfg)
      : cfg_{cfg},
        core_{core},
        memMgr_{cfg.pageSize},
        alloc_{memMgr_.core()},
        input_{cfg.maxInputBytes},
        lattice_{alloc_.get(), core->latticeConfig()},
        xtra_{alloc_.get(), core->dic().entries().entrySize()},
        outputManager_{alloc_.get(), &xtra_, &core->dic(), &lattice_} {}

  void reset() {
    memMgr_.reset();
    lattice_.reset();
    xtra_.reset();
  }

  Status analyze(StringPiece input) {
    reset();
    size_t maxCodepoints =
        std::min<size_t>({input.size(), cfg_.maxInputBytes,
                          std::numeric_limits<LatticePosition>::max()});
    latticeBldr_.reset(static_cast<LatticePosition>(maxCodepoints));
    JPP_RETURN_IF_ERROR(input_.reset(input));
    return Status::Ok();
  }
};

}  // analysis
}  // core
}  // jumanpp

#endif  // JUMANPP_ANALYZER_IMPL_H
