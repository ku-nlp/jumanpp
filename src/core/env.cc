//
// Created by Arseny Tolmachev on 2017/05/10.
//

#include "env.h"

namespace jumanpp {
namespace core {

Status JumanppEnv::loadModel(StringPiece filename) {
  JPP_RETURN_IF_ERROR(modelFile_.open(filename));
  JPP_RETURN_IF_ERROR(modelFile_.load(&modelInfo_));
  JPP_RETURN_IF_ERROR(dicBldr_.restoreDictionary(modelInfo_));
  JPP_RETURN_IF_ERROR(dicHolder_.load(dicBldr_));

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
    setRnnHolder(&rnnHolder_);
  }

  core_.reset(new CoreHolder{dicBldr_.spec, dicHolder_});

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

void JumanppEnv::setRnnHolder(analysis::RnnScorerGbeamFactory* holder) {
  if (!hasRnnModel()) {
    if (scorers_.others.empty()) {
      scorers_.others.push_back(holder);
      scorers_.scoreWeights.push_back(1.0f);
    }
    scoringConf_.numScorers += 1;
  }
  scorers_.others[0] = holder;
  if (scorers_.scoreWeights.size() == 2) {
    auto& conf = holder->config();
    scorers_.scoreWeights[0] = conf.perceptronWeight;
    scorers_.scoreWeights[1] = conf.rnnWeight;
  }
}

void JumanppEnv::setRnnConfig(
    const analysis::rnn::RnnInferenceConfig& rnnConf) {
  if (hasRnnModel()) {
    rnnHolder_.setConfig(rnnConf);
  }
  if (scorers_.scoreWeights.size() == 2) {
    scorers_.scoreWeights[0] = rnnConf.perceptronWeight;
    scorers_.scoreWeights[1] = rnnConf.rnnWeight;
  }
}

void JumanppEnv::setGlobalBeam(i32 globalBeam, i32 rightCheck, i32 rightBeam) {
  analyzerConfig_.globalBeamSize = globalBeam;
  analyzerConfig_.rightGbeamCheck = rightCheck;
  analyzerConfig_.rightGbeamSize = rightBeam;
}

}  // namespace core
}  // namespace jumanpp