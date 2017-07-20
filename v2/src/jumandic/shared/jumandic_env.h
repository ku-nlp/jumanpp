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
  std::unique_ptr<jumandic::output::OutputFormat> format;

  // use default values
  core::analysis::AnalyzerConfig analyzerConfig;

  // rnn
  rnn::mikolov::MikolovModelReader mikolovModel;
  core::analysis::rnn::RnnHolder rnnHolder;

  u64 numAnalyzed_ = 0;

 public:
  JumanppExec(const jumandic::JumanppConf& conf) : conf{conf} {}

  virtual Status initPerceptron() { return Status::Ok(); }

  virtual Status initRnn() { return Status::Ok(); }

  virtual Status initOutput();

  virtual Status init();

  Status analyze(StringPiece data, StringPiece comment = EMPTY_SP) {
    JPP_RETURN_IF_ERROR(analyzer.analyze(data));
    JPP_RETURN_IF_ERROR(format->format(analyzer, comment))
    numAnalyzed_ += 1;
    return Status::Ok();
  }

  StringPiece output() const { return format->result(); }

  u64 numAnalyzed() const { return numAnalyzed_; }

  virtual ~JumanppExec() = default;
};

}  // namespace jumandic
}  // namespace jumanpp

#endif  // JUMANPP_JUMANDIC_ENV_H
