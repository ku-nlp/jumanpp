//
// Created by Arseny Tolmachev on 2017/05/09.
//

#include "jumandic_env.h"
#include "jpp_jumandic_cg.h"

namespace jumanpp {
namespace jumandic {
Status JumanppExec::init() {
  JPP_RETURN_IF_ERROR(env.loadModel(conf.modelFile));

  bool newRnn = !conf.rnnModelFile.empty();

  if (!conf.rnnConfig.isDefault() && env.hasRnnModel() && !newRnn) {
    env.setRnnConfig(conf.rnnConfig);
  }

  if (newRnn) {
    JPP_RETURN_IF_ERROR(mikolovModel.open(conf.rnnModelFile));
    JPP_RETURN_IF_ERROR(mikolovModel.parse());
    JPP_RETURN_IF_ERROR(
        rnnHolder.init(mikolovModel, env.coreHolder()->dic(), "surface"));
    env.setRnnHolder(&rnnHolder);
  }

  jumanpp_generated::JumandicStatic features;
  JPP_RETURN_IF_ERROR(env.initFeatures(&features));
  JPP_RETURN_IF_ERROR(env.makeAnalyzer(&analyzer));
  JPP_RETURN_IF_ERROR(initOutput());
  return Status::Ok();
}
}  // namespace jumandic
}  // namespace jumanpp