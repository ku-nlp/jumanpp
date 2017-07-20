//
// Created by Arseny Tolmachev on 2017/05/09.
//

#include "jumandic_env.h"
#include "jpp_jumandic_cg.h"
#include "jumandic/shared/morph_format.h"
#include "jumandic/shared/subset_format.h"

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

Status JumanppExec::initOutput() {
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
    case OutputType::DicSubset:
      auto mfmt = new jumandic::output::SubsetFormat{};
      format.reset(mfmt);
      JPP_RETURN_IF_ERROR(mfmt->initialize(analyzer.output()));
      break;
  }
  return Status::Ok();
}
}  // namespace jumandic
}  // namespace jumanpp