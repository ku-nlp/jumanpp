//
// Created by Arseny Tolmachev on 2017/03/02.
//

#include "core/analysis/analyzer.h"
#include "core/analysis/analyzer_impl.h"

namespace jumanpp {
namespace core {
namespace analysis {

Analyzer::~Analyzer() {}

const OutputManager &Analyzer::output() const {
  return ptr_->output();
}

Status Analyzer::initialize(const CoreHolder *core, const AnalyzerConfig &cfg, ScoreConfig* scorer) {
  auto totalScorerSize = scorer->others.size() + 1;
  if (core->latticeConfig().scoreCnt != totalScorerSize) {
    return Status::InvalidParameter() << "number of scorers in core config does not match actual scorer number";
  }
  
  if (scorer->feature == nullptr) {
    return Status::InvalidParameter() << "main scorer is not specified";
  }
  
  if (scorer->scoreWeights.size() != totalScorerSize) {
    return Status::InvalidParameter() << "total scorer size does not match size of score weights";
  }

  pimpl_.reset(new AnalyzerImpl{core, cfg});
  ptr_ = pimpl_.get();
  scorer_ = scorer;
  return Status::Ok();
}

Status Analyzer::initialize(AnalyzerImpl *impl, ScoreConfig* scorer) {
  ptr_ = impl;
  scorer_ = scorer;
  return Status::Ok();
}

Status Analyzer::analyze(StringPiece input) {
  JPP_RETURN_IF_ERROR(ptr_->preprocess(input));
  JPP_RETURN_IF_ERROR(ptr_->buildLattice());
  ptr_->fixupLattice();
  JPP_RETURN_IF_ERROR(ptr_->computeScores(scorer_));
  return Status::Ok();
}

Analyzer::Analyzer(){}

}  // analysis
}  // core
}  // jumanpp
