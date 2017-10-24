//
// Created by Arseny Tolmachev on 2017/03/03.
//

#include "core/analysis/analyzer_impl.h"
#include "core/analysis/dictionary_node_creator.h"
#include "core/analysis/score_api.h"
#include "core/analysis/score_processor.h"
#include "core/analysis/unk_nodes_creator.h"
#include "core/impl/feature_impl_types.h"

namespace jumanpp {
namespace core {
namespace analysis {

Status AnalyzerImpl::resetForInput(StringPiece input) {
  reset();
  return setNewInput(input);
}

Status AnalyzerImpl::setNewInput(StringPiece input) {
  JPP_RETURN_IF_ERROR(input_.reset(input));
  latticeBldr_.reset(input_.numCodepoints());

  return Status::Ok();
}

AnalyzerImpl::AnalyzerImpl(const CoreHolder* core, const ScoringConfig& sconf,
                           const AnalyzerConfig& cfg)
    : cfg_{cfg},
      core_{core},
      memMgr_{cfg.pageSize},
      alloc_{memMgr_.core()},
      input_{cfg.maxInputBytes},
      latticeConfig_{core->latticeConfig(sconf)},
      lattice_{alloc_.get(), latticeConfig_},
      xtra_{alloc_.get(), core->dic().entries().entrySize(),
            core->runtime().unkMakers.numPlaceholders},
      outputManager_{alloc_.get(), &xtra_, &core->dic(), &lattice_},
      compactor_{core->dic().entries()} {
  ngramStats_.initialze(&core->runtime().features);
}

Status AnalyzerImpl::initScorers(const ScorerDef& cfg) {
  JPP_RETURN_IF_ERROR(core_->features().validate());

  if (cfg.scoreWeights.size() != latticeConfig_.scoreCnt) {
    return JPPS_INVALID_PARAMETER
           << "AnalyzerImpl: number of scorers (" << cfg.scoreWeights.size()
           << ") was different from number of score coefficients ("
           << latticeConfig_.scoreCnt << ")";
  }

  if (latticeConfig_.beamSize == 0) {
    return JPPS_INVALID_PARAMETER
           << "AnalyzerImpl: beam size can not be zero for scoring";
  }

  JPP_RETURN_IF_ERROR(compactor_.initialize(&xtra_, core_->runtime()));
  scorers_.clear();
  scorers_.reserve(cfg.others.size());
  for (auto& sf : cfg.others) {
    std::unique_ptr<ScoreComputer> comp;
    JPP_RETURN_IF_ERROR(sf->makeInstance(&comp));
    scorers_.emplace_back(std::move(comp));
  }
  return Status::Ok();
}

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
  analysis::UnkNodesContext unc{&xtra_, alloc()};
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
  analysis::UnkNodesContext unc{&xtra_, alloc()};
  for (auto& m : unk.stage2) {
    if (!m->spawnNodes(input_, &unc, &latticeBldr_)) {
      return Status::InvalidState() << "failed to create unk nodes (2)";
    }
  }

  return Status::Ok();
}

class InNodeFeatureComputer {
  const dic::DictionaryEntries entries_;
  const ExtraNodesContext& xtra_;
  const core::features::FeatureHolder& features_;
  features::impl::PrimitiveFeatureContext pfc_;

 public:
  InNodeFeatureComputer(const dic::DictionaryHolder& dic,
                        const features::FeatureHolder& features,
                        ExtraNodesContext* xtra)
      : entries_{dic.entries()},
        xtra_{*xtra},
        features_{features},
        pfc_{xtra, dic.fields()} {}

  bool importOneEntry(NodeInfo nfo, util::MutableArraySlice<i32> result) {
    auto ptr = nfo.entryPtr();
    if (ptr.isSpecial()) {
      auto node = xtra_.node(ptr);
      if (node->header.type == ExtraNodeType::Unknown) {
        auto nodeData = xtra_.nodeContent(node);
        util::copy_buffer(nodeData, result);
        auto hash = node->header.unk.contentHash;
        for (auto& e : result) {
          if (e < 0) {
            e = hash;
          }
        }
      } else if (node->header.type == ExtraNodeType::Alias) {
        auto nodeData = xtra_.nodeContent(node);
        util::copy_buffer(nodeData, result);
      } else {
        return false;
      }
    } else {  // dic node
      entries_.entryAtPtr(ptr.dicPtr()).fill(result, result.size());
    }
    return true;
  }

  Status importEntryData(LatticeBoundary* lb) {
    auto ptrs = lb->starts()->nodeInfo();
    auto entries = lb->starts()->entryData();
    for (int i = 0; i < ptrs.size(); ++i) {
      if (!importOneEntry(ptrs[i], entries.row(i))) {
        return Status::InvalidState()
               << "failed to import entry data into lattice";
      }
    }
    return Status::Ok();
  }

  void applyPrimitiveFeatures(LatticeBoundary* lb) {
    auto nodes = lb->starts();
    auto ptrs = nodes->nodeInfo();
    auto entries = nodes->entryData();
    auto primFeature = nodes->primitiveFeatureData();
    features::impl::PrimitiveFeatureData pfd{ptrs.data(), entries, primFeature};
    features_.primitive->applyBatch(&pfc_, &pfd);
  }

  void applyComputeFeatures(LatticeBoundary* lb) {
    auto nodes = lb->starts();
    auto ptrs = nodes->nodeInfo();
    auto entries = nodes->entryData();
    auto primFeature = nodes->primitiveFeatureData();
    features::impl::PrimitiveFeatureData pfd{ptrs.data(), entries, primFeature};
    features::impl::ComputeFeatureContext cfc;
    features_.compute->applyBatch(&cfc, &pfd);
  }

  void applyPatternFeatures(LatticeBoundary* lb) {
    auto nodes = lb->starts();
    auto primFeature = nodes->primitiveFeatureData();
    auto patFeature = nodes->patternFeatureData();
    features::impl::PatternFeatureData pfd{primFeature, patFeature};
    features_.pattern->applyBatch(&pfd);
  }
};

Status AnalyzerImpl::prepareNodeSeeds() {
  JPP_RETURN_IF_ERROR(makeNodeSeedsFromDic());
  JPP_RETURN_IF_ERROR(makeUnkNodes1());
  if (!checkLatticeConnectivity()) {
    JPP_RETURN_IF_ERROR(makeUnkNodes2());
    if (!checkLatticeConnectivity()) {
      return Status::InvalidState() << "could not build lattice";
    }
  }
  JPP_RETURN_IF_ERROR(latticeBldr_.prepare());
  return Status::Ok();
}

Status AnalyzerImpl::buildLattice() {
  lattice_.hintSize(input_.numCodepoints() + 3);

  LatticeConstructionContext lcc;
  InNodeFeatureComputer fc{core_->dic(), core_->features(), &xtra_};

  JPP_RETURN_IF_ERROR(latticeBldr_.makeBos(&lcc, &lattice_));
  JPP_DCHECK_EQ(lattice_.createdBoundaryCount(), 2);
  i32 totalBnds = input_.numCodepoints();
  for (i32 boundary = 0; boundary < totalBnds; ++boundary) {
    LatticeBoundary* bnd;
    latticeBldr_.compactBoundary(boundary, &compactor_);
    JPP_RETURN_IF_ERROR(
        latticeBldr_.constructSingleBoundary(&lattice_, &bnd, boundary));
    fc.importEntryData(bnd);
    if (latticeBldr_.isAccessible(boundary)) {
      fc.applyPrimitiveFeatures(bnd);
      fc.applyComputeFeatures(bnd);
      fc.applyPatternFeatures(bnd);
    }

    JPP_DCHECK_EQ(bnd->starts()->arraySize(),
                  latticeBldr_.infoAt(boundary).startCount);
  }

  JPP_RETURN_IF_ERROR(latticeBldr_.makeEos(&lcc, &lattice_));
  JPP_RETURN_IF_ERROR(latticeBldr_.fillEnds(&lattice_));
  JPP_DCHECK_EQ(totalBnds + 3, lattice_.createdBoundaryCount());

  return Status::Ok();
}

Status AnalyzerImpl::bootstrapAnalysis() {
  auto x = ScoreProcessor::make(this);
  JPP_RETURN_IF_ERROR(std::move(x.first));
  sproc_ = x.second;

  // bootstrap beam pointers
  auto beam0 = lattice_.boundary(0)->starts()->beamData().data();
  EntryBeam::initializeBlock(beam0);
  auto& bosRef = beam0.at(0);
  bosRef = ConnectionBeamElement{{0, 0, 0, 0, nullptr}, 0};
  auto beam1 = lattice_.boundary(1)->starts()->beamData().data();
  EntryBeam::initializeBlock(beam1);
  auto& bos1Ref = beam1.at(0);
  bos1Ref = ConnectionBeamElement{{1, 0, 0, 0, &bosRef.ptr}, 0};

  for (auto& s : scorers_) {
    s->preScore(&lattice_, &xtra_);
  }
  return Status::Ok();
}

Status AnalyzerImpl::computeScores(const ScorerDef* sconf) {
  JPP_DCHECK_NE(sconf, nullptr);
  JPP_DCHECK_NE(sconf->feature, nullptr);
  JPP_DCHECK_NE(sproc_, nullptr);
  JPP_DCHECK_EQ(sconf->others.size(), scorers_.size());

  auto bndCount = lattice_.createdBoundaryCount();
  if (bndCount <= 3) {  // 2xBOS + EOS
    return Status::Ok();
  }

  for (i32 boundary = 2; boundary < bndCount; ++boundary) {
    auto bnd = lattice_.boundary(boundary);
    JPP_DCHECK(bnd->endingsFilled());
    auto left = bnd->ends()->nodePtrs();
    auto& proc = *this->sproc_;

    EntryBeam::initializeBlock(bnd->starts()->beamData().data());

    proc.startBoundary(bnd->localNodeCount());
    proc.applyT0(boundary, sconf->feature);

    for (i32 t1idx = 0; t1idx < left.size(); ++t1idx) {
      auto& t1node = left[t1idx];
      proc.applyT1(t1node.boundary, t1node.position, sconf->feature);
      proc.resolveBeamAt(t1node.boundary, t1node.position);
      auto scores = bnd->scores();
      i32 activeBeam = proc.activeBeamSize();
      for (i32 beamIdx = 0; beamIdx < activeBeam; ++beamIdx) {
        proc.applyT2(beamIdx, sconf->feature);
        proc.copyFeatureScores(t1idx, beamIdx, scores);
      }
    }

    // use other scorers if any
    i32 scorerIdx = 1;
    for (auto& s : scorers_) {
      s->scoreBoundary(scorerIdx, &lattice_, boundary);
      scorerIdx += 1;
    }

    // Finally, make beams
    proc.makeBeams(boundary, bnd, sconf);
  }

  return Status::Ok();
}

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp