//
// Created by Arseny Tolmachev on 2017/05/10.
//

#include "env.h"

namespace jumanpp {
namespace core {

Status JumanppEnv::loadModel(StringPiece filename) {
  JPP_RETURN_IF_ERROR(modelFile_.open(filename));
  JPP_RETURN_IF_ERROR(modelFile_.load(&modelInfo_));
  JPP_RETURN_IF_ERROR(dicBldr_.restoreDictionary(modelInfo_, &runtime_));
  JPP_RETURN_IF_ERROR(dicHolder_.load(dicBldr_.result()));

  if (hasPerceptronModel()) {
    JPP_RETURN_IF_ERROR(perceptron_.load(modelInfo_));
    scoreConfig_.feature = &perceptron_;
    scoreConfig_.scoreWeights.push_back(1);
    coreConf_.numScorers += 1;
  }

  core_.reset(new CoreHolder{coreConf_, runtime_, dicHolder_});

  return Status::Ok();
}

bool JumanppEnv::hasPerceptronModel() const {
  for (auto& x : modelInfo_.parts) {
    if (x.kind == core::model::ModelPartKind::Perceprton) {
      return true;
    }
  }
  return false;
}

void JumanppEnv::setBeamSize(u32 size) {
  coreConf_.beamSize = size;
  core_->updateCoreConfig(coreConf_);
}

Status JumanppEnv::initFeatures(const features::StaticFeatureFactory* sff) {
  return core_->initialize(sff);
}

}  // namespace core
}  // namespace jumanpp