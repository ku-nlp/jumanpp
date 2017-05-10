//
// Created by Arseny Tolmachev on 2017/05/09.
//

#ifndef JUMANPP_JUMANDIC_ENV_H
#define JUMANPP_JUMANDIC_ENV_H

#include "core/analysis/analyzer.h"
#include "core/analysis/perceptron.h"
#include "core/impl/model_io.h"

namespace jumanpp {
namespace jumandic {

class JumandicEnv {
  core::CoreConfig coreConf_{1, 0};
  core::model::FilesystemModel modelFile_;
  core::model::ModelInfo modelInfo_;
  core::dic::DictionaryBuilder dicBldr_;
  core::RuntimeInfo runtime_;
  core::dic::DictionaryHolder dicHolder_;
  std::unique_ptr<core::CoreHolder> core_;

  core::analysis::HashedFeaturePerceptron perceptron_;

 public:
  Status loadModel(StringPiece filename);

  void setBeamSize(u32 size);

  const core::CoreHolder* coreHolder() const { return core_.get(); }
  const core::RuntimeInfo& runtimeInfo() const { return runtime_; }
  const core::spec::AnalysisSpec& spec() const { return dicBldr_.spec(); }
  bool hasPerceptronModel() const;

  Status initFeatures(const core::features::StaticFeatureFactory* pFactory);
};

}  // namespace jumandic
}  // namespace jumanpp

#endif  // JUMANPP_JUMANDIC_ENV_H
