//
// Created by Arseny Tolmachev on 2017/03/03.
//

#include "analyzer_impl.h"
#include "dictionary_node_creator.h"


namespace jumanpp {
namespace core {
namespace analysis {


Status AnalyzerImpl::resetForInput(StringPiece input) {
  reset();
  size_t maxCodepoints =
      std::min<size_t>({input.size(), cfg_.maxInputBytes,
                        std::numeric_limits<LatticePosition>::max()});
  latticeBldr_.reset(static_cast<LatticePosition>(maxCodepoints));
  JPP_RETURN_IF_ERROR(input_.reset(input));
  return Status::Ok();
}

AnalyzerImpl::AnalyzerImpl(const CoreHolder *core, const AnalyzerConfig &cfg)
    : cfg_{cfg},
      core_{core},
      memMgr_{cfg.pageSize},
      alloc_{memMgr_.core()},
      input_{cfg.maxInputBytes},
      lattice_{alloc_.get(), core->latticeConfig()},
      xtra_{alloc_.get(), core->dic().entries().entrySize()},
      outputManager_{alloc_.get(), &xtra_, &core->dic(), &lattice_} {}

Status AnalyzerImpl::makeNodeSeedsFromDic() {
  DictionaryNodeCreator dnc { core_->dic().entries() };
  if (!dnc.spawnNodes(input_, &latticeBldr_)) {
    return Status::InvalidState() << "error when creating nodes from dictionary";
  }
  return Status::Ok();
}


}  // analysis
}  // core
}  // jumanpp