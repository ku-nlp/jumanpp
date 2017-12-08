//
// Created by Arseny Tolmachev on 2017/03/03.
//

#include "core/analysis/analyzer_impl.h"
#include "core/analysis/dictionary_node_creator.h"
#include "core/analysis/innode_features.h"
#include "core/analysis/score_api.h"
#include "core/analysis/score_processor.h"
#include "core/analysis/unk_nodes_creator.h"
#include "core/impl/feature_impl_types.h"

namespace jumanpp {
namespace core {
namespace analysis {

Status AnalyzerImpl::resetForInput(StringPiece input) {
  reset();
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
      xtra_{alloc_.get(), core->dic().entries().numFeatures(),
            core->spec().features.numPlaceholders},
      outputManager_{&xtra_, &core->dic(), &lattice_},
      compactor_{core->dic().entries()} {
  ngramStats_.initialze(&core->spec().features);
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

  if (cfg_.rightGbeamCheck > 0 && cfg_.rightGbeamSize <= 0) {
    return JPPS_INVALID_PARAMETER
           << "right global beam size should not be zero if you enable it";
  }
  scorers_.clear();
  scorers_.reserve(cfg.others.size());
  for (auto& sf : cfg.others) {
    std::unique_ptr<ScoreComputer> comp;
    JPP_RETURN_IF_ERROR(sf->makeInstance(&comp));
    scorers_.emplace_back(std::move(comp));
  }

  if (!cfg_.storeAllPatterns && core().features().patternStatic) {
    auto& fspec = core_->spec().features;
    latticeConfig_.numFeaturePatterns =
        static_cast<u32>(fspec.pattern.size() - fspec.numUniOnlyPats);
  }

  if (cfg_.globalBeamSize > 0) {
    latticeConfig_.globalBeamSize = static_cast<u32>(cfg_.globalBeamSize);
  }

  if (!cfg.others.empty()) {
    if (cfg_.globalBeamSize <= 0) {
      return JPPS_INVALID_STATE << "additional scorers are supported only with "
                                   "global beam enabled";
    }
  }

  lattice_.updateConfig(latticeConfig_);
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
  analysis::UnkNodesContext unc{&xtra_, alloc(), dic().entries()};
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
  analysis::UnkNodesContext unc{&xtra_, alloc(), dic().entries()};
  for (auto& m : unk.stage2) {
    if (!m->spawnNodes(input_, &unc, &latticeBldr_)) {
      return Status::InvalidState() << "failed to create unk nodes (2)";
    }
  }

  return Status::Ok();
}

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
  InNodeFeatureComputer fc{core_->dic(), core_->features(), &xtra_, input_};

  JPP_RETURN_IF_ERROR(latticeBldr_.makeBos(&lcc, &lattice_));
  JPP_DCHECK_EQ(lattice_.createdBoundaryCount(), 2);
  i32 totalBnds = input_.numCodepoints();

  bool noStaticPattern = core_->features().patternStatic.get() == nullptr;

  for (i32 boundary = 0; boundary < totalBnds; ++boundary) {
    LatticeBoundary* bnd;
    JPP_RETURN_IF_ERROR(
        latticeBldr_.constructSingleBoundary(&lattice_, &bnd, boundary));

    if (noStaticPattern) {
      fc.importEntryData(bnd);
      if (latticeBldr_.isAccessible(boundary)) {
        fc.patternFeaturesDynamic(bnd);
      }
    }

    JPP_DCHECK_EQ(bnd->starts()->numEntries(),
                  latticeBldr_.infoAt(boundary).startCount);
  }

  JPP_RETURN_IF_ERROR(latticeBldr_.makeEos(&lcc, &lattice_));
  if (noStaticPattern) {
    fc.patternFeaturesEos(&lattice_);
  }
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

  return Status::Ok();
}

Status AnalyzerImpl::computeScoresFull(const ScorerDef* sconf) {
  JPP_DCHECK_NE(sconf, nullptr);
  JPP_DCHECK_NE(sconf->feature, nullptr);
  JPP_DCHECK_EQ(sconf->others.size(), scorers_.size());

  auto bndCount = lattice_.createdBoundaryCount();
  if (bndCount <= 3) {  // 2xBOS + EOS
    return Status::Ok();
  }

  for (i32 boundary = 2; boundary < bndCount; ++boundary) {
    JPP_CAPTURE(boundary);
    auto bnd = lattice_.boundary(boundary);
    JPP_DCHECK(bnd->endingsFilled());
    auto left = bnd->ends()->nodePtrs();
    auto& proc = *this->sproc_;

    EntryBeam::initializeBlock(bnd->starts()->beamData().data());

    proc.startBoundary(bnd->localNodeCount());
    if (proc.patternIsStatic()) {
      features::impl::PrimitiveFeatureContext pfc{
          &xtra_, dic().fields(), dic().entries(), input_.codepoints()};
      proc.computeT0All(boundary, sconf->feature, &pfc);
      if (JPP_UNLIKELY(cfg_.storeAllPatterns)) {
        proc.computeUniOnlyPatterns(boundary, &pfc);
      }
    } else {
      proc.applyT0(boundary, sconf->feature);
    }

    for (i32 t1idx = 0; t1idx < left.size(); ++t1idx) {
      JPP_CAPTURE(t1idx);
      auto& t1node = left[t1idx];
      proc.applyT1(t1node.boundary, t1node.position, sconf->feature);
      proc.resolveBeamAt(t1node.boundary, t1node.position);
      auto scores = bnd->scores();
      i32 activeBeam = proc.activeBeamSize();
      for (i32 beamIdx = 0; beamIdx < activeBeam; ++beamIdx) {
        JPP_CAPTURE(beamIdx);
        proc.applyT2(beamIdx, sconf->feature);
        proc.copyFeatureScores(t1idx, beamIdx, scores);
      }
    }

    // Finally, make beams
    proc.makeBeams(boundary, bnd, sconf);
  }

  return Status::Ok();
}

Status AnalyzerImpl::computeScoresGbeam(const ScorerDef* sconf) {
  JPP_DCHECK_NE(sconf, nullptr);
  JPP_DCHECK_NE(sconf->feature, nullptr);
  JPP_DCHECK_EQ(sconf->others.size(), scorers_.size());

  auto bndCount = lattice_.createdBoundaryCount();
  if (bndCount <= 3) {  // 2xBOS + EOS
    return Status::Ok();
  }

  auto& proc = *this->sproc_;

  for (i32 boundary = 2; boundary < bndCount; ++boundary) {
    JPP_CAPTURE(boundary);
    auto bnd = lattice_.boundary(boundary);
    if (bnd->localNodeCount() == 0) {
      continue;
    }
    JPP_DCHECK(bnd->endingsFilled());
    proc.startBoundary(bnd->localNodeCount());
    if (proc.patternIsStatic()) {
      auto entries = dic().entries();
      features::impl::PrimitiveFeatureContext pfc{&xtra_, dic().fields(),
                                                  entries, input_.codepoints()};
      proc.computeT0All(boundary, sconf->feature, &pfc);
      if (JPP_UNLIKELY(cfg_.storeAllPatterns)) {
        proc.computeUniOnlyPatterns(boundary, &pfc);
      }
    } else {
      proc.applyT0(boundary, sconf->feature);
    }

    auto gbeam = proc.makeGlobalBeam(boundary, cfg().globalBeamSize);
    proc.computeGbeamScores(boundary, gbeam, sconf->feature);
  }

  if (!scorers_.empty()) {
    u32 idx = 1;
    for (auto& s : scorers_) {
      s->scoreLattice(&lattice_, &xtra_, idx);
      ++idx;
    }
    proc.adjustBeamScores(sconf->scoreWeights);
    proc.remakeEosBeam(sconf->scoreWeights);
  }

  return Status::Ok();
}

Status AnalyzerImpl::computeScores(const ScorerDef* sconf) {
  if (sproc_ == nullptr) {
    return JPPS_INVALID_STATE << "Analyzer was not initialized";
  }
  if (cfg().globalBeamSize <= 0) {
    return computeScoresFull(sconf);
  } else {
    return computeScoresGbeam(sconf);
  }
}

bool AnalyzerImpl::setGlobalBeam(i32 leftBeam, i32 rightCheck, i32 rightBeam) {
  bool result = false;
  if (cfg_.globalBeamSize != leftBeam) {
    cfg_.globalBeamSize = leftBeam;
    latticeConfig_.globalBeamSize = static_cast<u32>(leftBeam);
    lattice_.updateConfig(latticeConfig_);
    result = true;
  }
  if (cfg_.rightGbeamSize != rightBeam) {
    cfg_.rightGbeamSize = rightBeam;
    result = true;
  }
  if (cfg_.rightGbeamCheck != rightCheck) {
    cfg_.rightGbeamCheck = rightCheck;
    result = true;
  }
  if (result) {
    sproc_ = nullptr;
  }
  return result;
}

bool AnalyzerImpl::setStoreAllPatterns(bool value) {
  if (cfg_.storeAllPatterns == value) {
    return false;
  }
  cfg_.storeAllPatterns = value;
  if (!cfg_.storeAllPatterns && core().features().patternStatic) {
    auto& fspec = core_->spec().features;
    latticeConfig_.numFeaturePatterns =
        static_cast<u32>(fspec.pattern.size() - fspec.numUniOnlyPats);
  } else {
    latticeConfig_.numFeaturePatterns =
        static_cast<u32>(core_->spec().features.pattern.size());
  }
  lattice_.updateConfig(latticeConfig_);
  return true;
}

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp