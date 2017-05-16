//
// Created by Arseny Tolmachev on 2017/05/10.
//

#ifndef JUMANPP_ENV_H
#define JUMANPP_ENV_H

#include "core/analysis/analyzer.h"
#include "core/analysis/perceptron.h"
#include "core/impl/model_io.h"

namespace jumanpp {
namespace core {

class JumanppEnv {
  ScoringConfig scoringConf_{1, 0};
  model::FilesystemModel modelFile_;
  model::ModelInfo modelInfo_;
  dic::DictionaryBuilder dicBldr_;
  RuntimeInfo runtime_;
  dic::DictionaryHolder dicHolder_;
  std::unique_ptr<core::CoreHolder> core_;

  analysis::AnalyzerConfig analyzerConfig_;

  analysis::HashedFeaturePerceptron perceptron_;
  analysis::ScorerDef scorers_;

 public:
  Status loadModel(StringPiece filename);

  Status initFeatures(const features::StaticFeatureFactory* pFactory);
  void setBeamSize(u32 size);

  const CoreHolder* coreHolder() const { return core_.get(); }
  const RuntimeInfo& runtimeInfo() const { return runtime_; }
  const spec::AnalysisSpec& spec() const { return dicBldr_.spec(); }
  bool hasPerceptronModel() const;

  Status makeAnalyzer(std::unique_ptr<analysis::Analyzer>* result) const {
    if (!hasPerceptronModel()) {
      return Status::InvalidState() << "loaded model (" << modelFile_.name()
                                    << ") does not have was not trained";
    }
    result->reset(new analysis::Analyzer());
    return (*result)->initialize(coreHolder(), analyzerConfig_, scoringConf_,
                                 &scorers_);
  }
};

}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_ENV_H
