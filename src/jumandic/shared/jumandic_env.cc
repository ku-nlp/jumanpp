//
// Created by Arseny Tolmachev on 2017/05/09.
//

#include "jumandic_env.h"
#include "core/analysis/analyzer_impl.h"
#include "core/impl/global_beam_position_fmt.h"
#include "core/impl/graphviz_format.h"
#include "core_version.h"
#include "jpp_jumandic_cg.h"
#include "jumandic/shared/lattice_format.h"
#include "jumandic/shared/morph_format.h"
#include "jumandic/shared/subset_format.h"
#include "util/format.h"

#include <fstream>
#include <iostream>

namespace jumanpp {
namespace jumandic {
Status JumanppExec::init() {
  JPP_RETURN_IF_ERROR(env.loadModel(conf.modelFile.value()));
  env.setBeamSize(conf.beamSize);
  env.setGlobalBeam(conf.globalBeam, conf.rightCheck, conf.rightBeam);

  bool newRnn = !conf.rnnModelFile.value().empty();

  if (!conf.rnnConfig.isDefault() && env.hasRnnModel() && !newRnn) {
    env.setRnnConfig(conf.rnnConfig);
  }

  if (newRnn) {
    JPP_RETURN_IF_ERROR(rnnFactory.make(
        conf.rnnModelFile.value(), env.coreHolder()->dic(), conf.rnnConfig));
    env.setRnnHolder(&rnnFactory);
  }

  jumanpp_generated::JumandicStatic features;
  JPP_RETURN_IF_ERROR(env.initFeatures(&features));
  JPP_RETURN_IF_ERROR(env.makeAnalyzer(&analyzer));
  JPP_RETURN_IF_ERROR(initOutput());
  return Status::Ok();
}

Status JumanppExec::initOutput() {
  switch (conf.outputType.value()) {
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
    case jumandic::OutputType::FullMorph: {
      auto mfmt = new jumandic::output::MorphFormat(true);
      format.reset(mfmt);
      JPP_RETURN_IF_ERROR(mfmt->initialize(analyzer.output()));
      break;
    }
    case OutputType::DicSubset: {
      auto mfmt = new jumandic::output::SubsetFormat{};
      format.reset(mfmt);
      JPP_RETURN_IF_ERROR(mfmt->initialize(analyzer.output()));
      break;
    }
    case OutputType::Lattice: {
      auto mfmt = new jumandic::output::LatticeFormat{conf.beamOutput};
      format.reset(mfmt);
      JPP_RETURN_IF_ERROR(mfmt->initialize(analyzer.output()));
      break;
    }
    case OutputType::Version:
    case OutputType::ModelInfo:
      return Status::Ok();
#ifdef JPP_ENABLE_DEV_TOOLS
    case OutputType::GlobalBeamPos: {
      auto mfmt = new core::output::GlobalBeamPositionFormat{conf.globalBeam};
      format.reset(mfmt);
      JPP_RETURN_IF_ERROR(mfmt->initialize(analyzer));
    }
#endif
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

  auto filename =
      fmt::format("{0}/{1:08}.dot", conf.graphvizDir.value(), numAnalyzed_);
  std::ofstream of{filename};
  if (of) {
    of << fmt.result() << "\n";
  }
  return Status::Ok();
}

void JumanppExec::printModelInfo() const {
  core::model::FilesystemModel model;
  model.open(this->conf.modelFile.value());
  model.renderInfo();
}

void JumanppExec::printFullVersion() const {
  core::model::FilesystemModel model;
  std::cout << "Juman++ Version: " << core::JPP_VERSION_STRING;
  core::model::ModelInfo mi;
  if (!model.open(conf.modelFile.value()) || !model.load(&mi)) {
    std::cout << "\n";
    return;
  }

  auto dic = mi.firstPartOf(core::model::ModelPartKind::Dictionary);
  if (dic && !dic->comment.empty()) {
    std::cout << " / Dictionary: " << dic->comment;
  }

  auto perc = mi.firstPartOf(core::model::ModelPartKind::Perceprton);
  if (perc && !perc->comment.empty()) {
    std::cout << " / LM: " << perc->comment;
  }

  auto rnn = mi.firstPartOf(core::model::ModelPartKind::Rnn);
  if (rnn && !rnn->comment.empty()) {
    std::cout << " / RNN:" << rnn->comment;
  }

  std::cout << "\n";
}

StringPiece JumanppExec::emptyResult() const {
  switch (conf.outputType.value()) {
    case OutputType::Juman:
    case OutputType::Lattice:
      return "# ERROR\nEOS\n";
    case OutputType::Morph:
    case OutputType::FullMorph:
      return "# ERROR\n";
    default:
      return EMPTY_SP;
  }
}

}  // namespace jumandic
}  // namespace jumanpp