//
// Created by Arseny Tolmachev on 2017/05/09.
//

#ifndef JUMANPP_JUMANDIC_ENV_H
#define JUMANPP_JUMANDIC_ENV_H

#include "core/analysis/perceptron.h"
#include "core/analysis/rnn_scorer.h"
#include "core/analysis/score_api.h"
#include "core/env.h"
#include "core/impl/model_io.h"
#include "jumandic/shared/juman_format.h"
#include "jumandic/shared/jumandic_id_resolver.h"
#include "jumandic/shared/jumanpp_args.h"
#include "rnn/mikolov_rnn.h"

namespace jumanpp {
namespace jumandic {

class JumanppExec {
 protected:
  jumandic::JumanppConf conf;
  core::JumanppEnv env;
  core::analysis::Analyzer analyzer_;
  std::unique_ptr<core::OutputFormat> format_;
  jumandic::JumandicIdResolver idResolver_;

  // rnn
  core::analysis::RnnScorerGbeamFactory rnnFactory;

  u64 numAnalyzed_ = 0;

  Status writeGraphviz();

 public:
  JumanppExec() = default;
  explicit JumanppExec(const jumandic::JumanppConf& conf) : conf{conf} {}

  virtual Status initOutput();

  virtual Status init(const jumandic::JumanppConf& conf) {
    this->conf.mergeWith(conf);
    return init();
  }

  virtual Status init();

  Status analyze(StringPiece data, StringPiece comment = EMPTY_SP) {
    try {
      JPP_RETURN_IF_ERROR(analyzer_.analyze(data));
      JPP_RETURN_IF_ERROR(format_->format(analyzer_, comment))
      if (!conf.graphvizDir.value().empty()) {
        JPP_RETURN_IF_ERROR(writeGraphviz());
      }
      numAnalyzed_ += 1;
      return Status::Ok();
    } catch (std::exception& e) {
      return JPPS_INVALID_STATE << "failed to analyze [" << data << "] ["
                                << comment << "]: " << e.what();
    }
  }

  StringPiece output() const { return format_->result(); }

  u64 numAnalyzed() const { return numAnalyzed_; }

  virtual ~JumanppExec() = default;

  void printModelInfo() const;

  void printFullVersion() const;
  StringPiece emptyResult() const;
  core::analysis::Analyzer* analyzerPtr() { return &analyzer_; }
  core::OutputFormat* format() { return format_.get(); }
  const core::CoreHolder& core() const { return *env.coreHolder(); }
  Status initAnalyzer(core::analysis::Analyzer* result);
};

const core::features::StaticFeatureFactory* jumandicStaticFeatures();

}  // namespace jumandic
}  // namespace jumanpp

#endif  // JUMANPP_JUMANDIC_ENV_H
