//
// Created by Arseny Tolmachev on 2017/05/09.
//

#include "jumandic_env.h"
#include "jpp_jumandic_cg.h"

namespace jumanpp {
namespace jumandic {
Status JumanppExec::init() {
  jumanpp_generated::JumandicStatic features;
  JPP_RETURN_IF_ERROR(env.loadModel(conf.modelFile));
  JPP_RETURN_IF_ERROR(env.initFeatures(&features));
  JPP_RETURN_IF_ERROR(env.makeAnalyzer(&analyzer));
  JPP_RETURN_IF_ERROR(initOutput());
  return Status::Ok();
}
}  // namespace jumandic
}  // namespace jumanpp