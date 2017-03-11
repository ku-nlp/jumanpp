//
// Created by Arseny Tolmachev on 2017/03/10.
//

#ifndef JUMANPP_JUMANPP_H
#define JUMANPP_JUMANPP_H

#include <random>
#include "core/analysis/perceptron.h"
#include "core/analysis/score_api.h"
#include "core/impl/model_io.h"
#include "jumandic/shared/juman_format.h"

namespace jumanpp {

class JumanppExec {
 protected:
  core::model::FilesystemModel model;
  jumandic::output::JumanFormat format;
  core::model::ModelInfo modelInfo;
  core::dic::DictionaryBuilder dicbldr;
  core::RuntimeInfo runtimeInfo;
  core::dic::DictionaryHolder dicHolder;
  core::CoreConfig coreConf{
      5,  // beamSize
      1,  // numScorers
  };
  std::unique_ptr<core::CoreHolder> coreHolder;
  // loaded model up to here

  std::vector<float> perceptronWeights;
  core::analysis::ScoreConfig scorers;

  // use default values
  core::analysis::AnalyzerConfig analyzerConfig;
  core::analysis::Analyzer analyzer;

 public:
  virtual Status initPerceptron() {
    std::default_random_engine rng{0xfeed};
    std::normal_distribution<float> dirst{0, 0.001};
    constexpr size_t perc_size = 4 * 1024 * 1024;
    perceptronWeights.resize(perc_size);
    for (size_t i = 0; i < perceptronWeights.size(); ++i) {
      perceptronWeights[i] = dirst(rng);
    }
    scorers.scoreWeights.push_back(1.0f);
    return Status::Ok();
  }

  virtual Status init(StringPiece modelFile) {
    JPP_RETURN_IF_ERROR(model.open(modelFile));
    JPP_RETURN_IF_ERROR(model.load(&modelInfo));
    JPP_RETURN_IF_ERROR(dicbldr.restoreDictionary(modelInfo, &runtimeInfo));
    JPP_RETURN_IF_ERROR(dicHolder.load(dicbldr.result()));
    coreHolder.reset(new core::CoreHolder{coreConf, runtimeInfo, dicHolder});
    // inject static features here
    JPP_RETURN_IF_ERROR(coreHolder->initialize(nullptr));
    JPP_RETURN_IF_ERROR(initPerceptron());
    scorers.feature =
        new core::analysis::HashedFeaturePerceptron(perceptronWeights);
    JPP_RETURN_IF_ERROR(
        analyzer.initialize(coreHolder.get(), analyzerConfig, &scorers));
    JPP_RETURN_IF_ERROR(format.initialize(analyzer.output()));
    return Status::Ok();
  }

  Status analyze(StringPiece data) {
    JPP_RETURN_IF_ERROR(analyzer.analyze(data));
    JPP_RETURN_IF_ERROR(format.format(analyzer));
    return Status::Ok();
  }

  StringPiece output() const { return format.result(); }

  virtual ~JumanppExec() {}
};
}

#endif  // JUMANPP_JUMANPP_H
