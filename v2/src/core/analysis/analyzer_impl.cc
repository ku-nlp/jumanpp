//
// Created by Arseny Tolmachev on 2017/03/03.
//

#include "analyzer_impl.h"
#include "core/analysis/unk_nodes_creator.h"
#include "dictionary_node_creator.h"

namespace jumanpp {
namespace core {
namespace analysis {

Status AnalyzerImpl::resetForInput(StringPiece input) {
  reset();
  JPP_RETURN_IF_ERROR(input_.reset(input));
  latticeBldr_.reset(input_.numCodepoints());

  return Status::Ok();
}

AnalyzerImpl::AnalyzerImpl(const CoreHolder* core, const AnalyzerConfig& cfg)
    : cfg_{cfg},
      core_{core},
      memMgr_{cfg.pageSize},
      alloc_{memMgr_.core()},
      input_{cfg.maxInputBytes},
      lattice_{alloc_.get(), core->latticeConfig()},
      xtra_{alloc_.get(), core->dic().entries().entrySize(),
            core->runtime().unkMakers.numPlaceholders},
      outputManager_{alloc_.get(), &xtra_, &core->dic(), &lattice_} {}

Status AnalyzerImpl::makeNodeSeedsFromDic() {
  DictionaryNodeCreator dnc{core_->dic().entries()};
  if (!dnc.spawnNodes(input_, &latticeBldr_)) {
    return Status::InvalidState()
           << "error when creating nodes from dictionary";
  }
  return Status::Ok();
}

Status AnalyzerImpl::makeUnkNodes1() {
  auto& unk = core_->unkMakers();
  analysis::UnkNodesContext unc{&xtra_};
  for (auto& m : unk.stage1) {
    if (!m->spawnNodes(input_, &unc, &latticeBldr_)) {
      return Status::InvalidState() << "failed to create unk nodes";
    }
  }

  return Status::Ok();
}

bool AnalyzerImpl::checkLatticeConnectivity() {
  return latticeBldr_.checkConnectability();
}

Status AnalyzerImpl::makeUnkNodes2() {
  auto& unk = core_->unkMakers();
  analysis::UnkNodesContext unc{&xtra_};
  for (auto& m : unk.stage2) {
    if (!m->spawnNodes(input_, &unc, &latticeBldr_)) {
      return Status::InvalidState() << "failed to create unk nodes (2)";
    }
  }

  return Status::Ok();
}

Status AnalyzerImpl::buildLattice() {
  JPP_RETURN_IF_ERROR(makeUnkNodes1());
  if (!checkLatticeConnectivity()) {
    JPP_RETURN_IF_ERROR(makeUnkNodes2());
    if (!checkLatticeConnectivity()) {
      return Status::InvalidState() << "could not build lattice";
    }
  }

  LatticeConstructionContext lcc;

  JPP_RETURN_IF_ERROR(latticeBldr_.makeBos(&lcc, &lattice_));

  return Status::NotImplemented();
}

}  // analysis
}  // core
}  // jumanpp