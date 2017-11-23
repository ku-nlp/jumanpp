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
#include "jumandic/main/jumanpp_args.h"
#include "jumandic/shared/juman_format.h"
#include "rnn/mikolov_rnn.h"

namespace jumanpp {
namespace jumandic {

class JumanppExec {
 protected:
  jumandic::JumanppConf conf;
  core::JumanppEnv env;
  core::analysis::Analyzer analyzer;
  std::unique_ptr<core::OutputFormat> format;

  // rnn
  core::analysis::RnnScorerGbeamFactory rnnFactory;

  u64 numAnalyzed_ = 0;

  Status writeGraphviz();

 public:
  explicit JumanppExec(const jumandic::JumanppConf& conf) : conf{conf} {}

  virtual Status initOutput();

  virtual Status init();

  Status analyze(StringPiece data, StringPiece comment = EMPTY_SP) {
    try {
      JPP_RETURN_IF_ERROR(analyzer.analyze(data));
      JPP_RETURN_IF_ERROR(format->format(analyzer, comment))
      if (!conf.graphvizDir.value().empty()) {
        writeGraphviz();
      }
      numAnalyzed_ += 1;
      return Status::Ok();
    } catch (std::exception& e) {
      return JPPS_INVALID_STATE << "failed to analyze [" << data << "] ["
                                << comment << "]: " << e.what();
    }
  }

  StringPiece output() const { return format->result(); }

  u64 numAnalyzed() const { return numAnalyzed_; }

  virtual ~JumanppExec() = default;

  void printModelInfo() const;
};

}  // namespace jumandic
}  // namespace jumanpp

#endif  // JUMANPP_JUMANDIC_ENV_H
