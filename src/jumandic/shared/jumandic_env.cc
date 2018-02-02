//
// Created by Arseny Tolmachev on 2017/05/09.
//

#include "jumandic_env.h"
#include "core/analysis/analyzer_impl.h"
#include "core/impl/global_beam_position_fmt.h"
#include "core/impl/graphviz_format.h"
#include "core/impl/segmented_format.h"
#include "core_version.h"
#include "jpp_jumandic_cg.h"
#include "jumandic/shared/lattice_format.h"
#include "jumandic/shared/morph_format.h"
#include "jumandic/shared/subset_format.h"

#if defined(JPP_USE_PROTOBUF)
#include "core/proto/lattice_dump_output.h"
#endif

#include <fstream>
#include <iostream>

namespace jumanpp {
namespace jumandic {
Status JumanppExec::init() {
  JPP_RETURN_IF_ERROR(env.loadModel(conf.modelFile.value()));
  env.setBeamSize(conf.beamSize);
  env.setGlobalBeam(conf.globalBeam, conf.rightCheck, conf.rightBeam);
  if (conf.autoStep.defined()) {
    env.setAutoBeam(conf.beamSize, conf.autoStep, conf.globalBeam);
  }

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
  JPP_RETURN_IF_ERROR(env.makeAnalyzer(&analyzer_));
  JPP_RETURN_IF_ERROR(initOutput());
  return Status::Ok();
}

Status JumanppExec::initOutput() {
  switch (conf.outputType.value()) {
    case jumandic::OutputType::Juman: {
      auto jfmt = new jumandic::output::JumanFormat;
      format_.reset(jfmt);
      JPP_RETURN_IF_ERROR(jfmt->initialize(analyzer_.output()));
      break;
    }
    case jumandic::OutputType::Morph: {
      auto mfmt = new jumandic::output::MorphFormat(false);
      format_.reset(mfmt);
      JPP_RETURN_IF_ERROR(mfmt->initialize(analyzer_.output()));
      break;
    }
    case jumandic::OutputType::FullMorph: {
      auto mfmt = new jumandic::output::MorphFormat(true);
      format_.reset(mfmt);
      JPP_RETURN_IF_ERROR(mfmt->initialize(analyzer_.output()));
      break;
    }
    case OutputType::DicSubset: {
      auto mfmt = new jumandic::output::SubsetFormat{};
      format_.reset(mfmt);
      JPP_RETURN_IF_ERROR(mfmt->initialize(analyzer_.output()));
      break;
    }
    case OutputType::Lattice: {
      auto mfmt = new jumandic::output::LatticeFormat{conf.beamOutput};
      format_.reset(mfmt);
      JPP_RETURN_IF_ERROR(mfmt->initialize(analyzer_.output()));
      break;
    }
    case OutputType::Segmentation: {
      auto mfmt = new core::output::SegmentedFormat{};
      format_.reset(mfmt);
      JPP_RETURN_IF_ERROR(mfmt->initialize(analyzer_.output(),
                                           *env.coreHolder(),
                                           conf.segmentSeparator.value()));
      break;
    }
    case OutputType::Version:
    case OutputType::ModelInfo:
      return Status::Ok();
#ifdef JPP_ENABLE_DEV_TOOLS
    case OutputType::GlobalBeamPos: {
      auto mfmt = new core::output::GlobalBeamPositionFormat{conf.globalBeam};
      format_.reset(mfmt);
      JPP_RETURN_IF_ERROR(mfmt->initialize(analyzer_));
      break;
    }
#if defined(JPP_USE_PROTOBUF)
    case OutputType::FullLatticeDump: {
      auto mfmt = new core::output::LatticeDumpOutput;
      format_.reset(mfmt);
      JPP_RETURN_IF_ERROR(
          mfmt->initialize(analyzer_.impl(), &env.featureScorer()->weights()));
      analyzer_.impl()->setStoreAllPatterns(true);
      break;
    }
#endif
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
  auto beamSize = analyzer_.impl()->autoBeamSizes();
  if (beamSize == 0) {
    beamSize = conf.beamSize;
  }
  JPP_RETURN_IF_ERROR(gb.build(&fmt, beamSize));
  JPP_RETURN_IF_ERROR(fmt.initialize(analyzer_.output()));
  JPP_RETURN_IF_ERROR(fmt.render(analyzer_.impl()->lattice()));

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
  Status s = model.open(this->conf.modelFile.value());
  if (s) {
    model.renderInfo();
  }
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
      return StringPiece{"# ERROR\nEOS\n"};
    case OutputType::Morph:
    case OutputType::FullMorph:
      return StringPiece{"# ERROR\n"};
    default:
      return EMPTY_SP;
  }
}

}  // namespace jumandic
}  // namespace jumanpp