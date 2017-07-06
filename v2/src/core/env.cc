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
    scorers_.feature = &perceptron_;
    scorers_.scoreWeights.push_back(1);
    scoringConf_.numScorers += 1;
  }

  if (hasRnnModel()) {
    JPP_RETURN_IF_ERROR(rnnHolder_.load(modelInfo_));
    scorers_.others.push_back(&rnnHolder_);
    scorers_.scoreWeights.push_back(1);
    scoringConf_.numScorers += 1;
  }

  core_.reset(new CoreHolder{runtime_, dicHolder_});

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

void JumanppEnv::setBeamSize(u32 size) { scoringConf_.beamSize = size; }

Status JumanppEnv::initFeatures(const features::StaticFeatureFactory* sff) {
  return core_->initialize(sff);
}

bool JumanppEnv::hasRnnModel() const {
  auto it = std::find_if(modelInfo_.parts.begin(), modelInfo_.parts.end(),
                         [](const model::ModelPart& p) {
                           return p.kind == model::ModelPartKind::Rnn;
                         });
  return it != modelInfo_.parts.end();
}

}  // namespace core
}  // namespace jumanpp