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
#include "jumandic/shared/morph_format.h"
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

public:
  JumanppExec(const jumandic::JumanppConf& conf) : conf{conf} {}

  virtual Status initPerceptron() {
    //    std::default_random_engine rng{0xfeed};
    //    std::normal_distribution<float> dirst{0, 0.001};
    //    constexpr size_t perc_size = 4 * 1024 * 1024;
    //    perceptronWeights.resize(perc_size);
    //    for (size_t i = 0; i < perceptronWeights.size(); ++i) {
    //      perceptronWeights[i] = dirst(rng);
    //    }
    //    scorers.scoreWeights.push_back(1.0f);
    return Status::Ok();
  }

  virtual Status initRnn() {
    //    if (conf.rnnModelFile.size() > 0) {
    //      JPP_RETURN_IF_ERROR(mikolovModel.open(conf.rnnModelFile));
    //      JPP_RETURN_IF_ERROR(mikolovModel.parse());
    //      JPP_RETURN_IF_ERROR(rnnHolder.init(mikolovModel));
    //      scorers.scoreWeights.push_back(1.0f);
    //      scorers.others.push_back(&rnnHolder);
    //      coreConf.numScorers += 1;
    //    }

    return Status::Ok();
  }

  virtual Status initOutput() {
    switch (conf.outputType) {
      case jumandic::OutputType::Juman: {
        auto jfmt = new jumandic::output::JumanFormat;
        format.reset(jfmt);
        JPP_RETURN_IF_ERROR(jfmt->initialize(analyzer.output()));
        break;
      }
      case jumandic::OutputType::Morph: {
        auto mfmt = new jumandic::output::MorphFormat(false);
        format.reset(mfmt);
        JPP_RETURN_IF_ERROR(mfmt->initialize(analyzer.output()));
        break;
      }
    }
    return Status::Ok();
  }

  virtual Status init();

  Status analyze(StringPiece data, StringPiece comment = EMPTY_SP) {
    JPP_RETURN_IF_ERROR(analyzer.analyze(data));
    JPP_RETURN_IF_ERROR(format->format(analyzer, comment))
    return Status::Ok();
  }

  StringPiece output() const { return format->result(); }

  virtual ~JumanppExec() = default;
};


}  // namespace jumandic
}  // namespace jumanpp

#endif  // JUMANPP_JUMANDIC_ENV_H
