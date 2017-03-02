//
// Created by Arseny Tolmachev on 2017/03/02.
//

#include "analyzer.h"
#include "core/analysis/analysis_input.h"
#include "core/analysis/lattice_types.h"

namespace jumanpp {
namespace core {
namespace analysis {

class Analyzer {
  AnalyzerConfig cfg_;
  const CoreHolder* core_;
  util::memory::Manager memMgr_;
  std::unique_ptr<util::memory::ManagedAllocatorCore> alloc_;
  AnalysisInput input_;
  Lattice lattice_;
  ExtraNodesContext xtra_;
  OutputManager outputManager_;

public:
  Analyzer(const Analyzer&) = delete;
  Analyzer(Analyzer&&) = delete;

  Analyzer(const CoreHolder* core, const AnalyzerConfig& cfg)
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
    JPP_RETURN_IF_ERROR(input_.reset(input));
    return Status::Ok();
  }
};


}  // analysis
}  // core
}  // jumanpp

