//
// Created by Arseny Tolmachev on 2017/05/09.
//

#include "jumandic_env.h"

namespace jumanpp {
namespace jumandic {

Status JumandicEnv::loadModel(StringPiece filename) {
  JPP_RETURN_IF_ERROR(modelFile_.open(filename));
  JPP_RETURN_IF_ERROR(modelFile_.load(&modelInfo_));
  JPP_RETURN_IF_ERROR(dicBldr_.restoreDictionary(modelInfo_, &runtime_));
  JPP_RETURN_IF_ERROR(dicHolder_.load(dicBldr_.result()));

  if (hasPerceptronModel()) {
    JPP_RETURN_IF_ERROR(perceptron_.load(modelInfo_));
    coreConf_.numScorers += 1;
  }

  core_.reset(new core::CoreHolder{coreConf_, runtime_, dicHolder_});

  return Status::Ok();
}

bool JumandicEnv::hasPerceptronModel() const {
  for (auto& x : modelInfo_.parts) {
    if (x.kind == core::model::ModelPartKind::Perceprton) {
      return true;
    }
  }
  return false;
}

void JumandicEnv::setBeamSize(u32 size) {
  coreConf_.beamSize = size;
  core_->updateCoreConfig(coreConf_);
}

Status JumandicEnv::initFeatures(
    const core::features::StaticFeatureFactory* sff) {
  return core_->initialize(sff);
}

}  // namespace jumandic
}  // namespace jumanpp