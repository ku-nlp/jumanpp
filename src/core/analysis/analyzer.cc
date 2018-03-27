//
// Created by Arseny Tolmachev on 2017/03/02.
//

#include "core/analysis/analyzer.h"
#include "core/analysis/analyzer_impl.h"

namespace jumanpp {
namespace core {
namespace analysis {

Analyzer::~Analyzer() {}

const OutputManager &Analyzer::output() const { return ptr_->output(); }

Status Analyzer::initialize(const CoreHolder *core, const AnalyzerConfig &cfg,
                            const ScoringConfig &scoreConf,
                            const ScorerDef *scorer) {
  auto totalScorerSize = scorer->others.size() + 1;
  if (scoreConf.numScorers != totalScorerSize) {
    return JPPS_INVALID_PARAMETER << "number of scorers in core config "
                                     "does not match actual scorer number";
  }

  if (scorer->feature == nullptr) {
    return JPPS_INVALID_PARAMETER << "main scorer is not specified";
  }

  if (scorer->scoreWeights.size() != totalScorerSize) {
    return JPPS_INVALID_PARAMETER
           << "total scorer size does not match size of score weights";
  }

  pimpl_.reset(new AnalyzerImpl{core, scoreConf, cfg});
  return initialize(pimpl_.get(), scorer);
}

Status Analyzer::initialize(AnalyzerImpl *impl, const ScorerDef *scorer) {
  JPP_RETURN_IF_ERROR(impl->initScorers(*scorer));
  ptr_ = impl;
  scorer_ = scorer;
  return Status::Ok();
}

Status Analyzer::analyze(StringPiece input, ScorePlugin *plugin) {
  JPP_RETURN_IF_ERROR(ptr_->resetForInput(input));
  ptr_->setPlugin(plugin);
  JPP_RETURN_IF_ERROR(ptr_->prepareNodeSeeds());
  JPP_RETURN_IF_ERROR(ptr_->buildLattice());
  JPP_RETURN_IF_ERROR(ptr_->bootstrapAnalysis());
  JPP_RETURN_IF_ERROR(ptr_->computeScores(scorer_));
  return Status::Ok();
}

Analyzer::Analyzer() {}

const CoreHolder &Analyzer::core() const { return ptr_->core(); }

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp
