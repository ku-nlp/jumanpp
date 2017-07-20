//
// Created by Arseny Tolmachev on 2017/05/09.
//

#include "jumandic_env.h"
#include "core/analysis/analyzer_impl.h"
#include "core/impl/graphviz_format.h"
#include "jpp_jumandic_cg.h"
#include "jumandic/shared/morph_format.h"
#include "jumandic/shared/subset_format.h"

#include <fstream>

namespace jumanpp {
namespace jumandic {
Status JumanppExec::init() {
  JPP_RETURN_IF_ERROR(env.loadModel(conf.modelFile));
  env.setBeamSize(conf.beamSize);

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

Status JumanppExec::writeGraphviz() {
  core::format::GraphVizBuilder gb;
  gb.row({"canonic"});
  gb.row({"surface"});
  gb.row({"pos", "subpos"});
  gb.row({"conjform", "conjtype"});
  core::format::GraphVizFormat fmt;
  JPP_RETURN_IF_ERROR(gb.build(&fmt, conf.beamSize));
  JPP_RETURN_IF_ERROR(fmt.initialize(analyzer.output()));
  JPP_RETURN_IF_ERROR(fmt.render(analyzer.impl()->lattice()));

  char filename[512];
  filename[0] = 0;
  snprintf(filename, 510, "%s/%08lld.dot", conf.graphvizDir.c_str(),
           numAnalyzed_);
  std::ofstream of{filename};
  if (of) {
    of << fmt.result() << "\n";
  }
  return Status::Ok();
}

}  // namespace jumandic
}  // namespace jumanpp