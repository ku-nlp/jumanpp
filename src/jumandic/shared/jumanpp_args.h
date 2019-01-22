//
// Created by Arseny Tolmachev on 2017/03/11.
//

#ifndef JUMANPP_JUMANPP_ARGS_H
#define JUMANPP_JUMANPP_ARGS_H

#include <ostream>
#include <string>
#include "core/analysis/rnn_scorer.h"
#include "core_config.h"
#include "util/cfg.h"
#include "util/status.hpp"

namespace jumanpp {
namespace jumandic {

enum class OutputType {
  Invalid,
  Version,
  ModelInfo,
  Segmentation,
  Juman,
  Morph,
  FullMorph,
  DicSubset,
  Lattice,
#if defined(JPP_USE_PROTOBUF)
  JumanPb,
  LatticePb,
  FullLatticeDump
#endif
#if defined(JPP_ENABLE_DEV_TOOLS)
      GlobalBeamPos,
#endif
};

enum class InputType { Raw, PartiallyAnnotated };

struct JumanppConf {
  util::Cfg<std::string> configFile;
  util::Cfg<std::string> modelFile;
  util::Cfg<OutputType> outputType = OutputType::Juman;
  util::Cfg<InputType> inputType = InputType::Raw;
  util::Cfg<std::string> outputFile{"-"};
  util::Cfg<std::vector<std::string>> inputFiles{};
  util::Cfg<std::string> rnnModelFile;
  core::analysis::rnn::RnnInferenceConfig rnnConfig{};
  util::Cfg<std::string> graphvizDir;
  util::Cfg<i32> beamSize = 5;
  util::Cfg<i32> beamOutput = 1;
  util::Cfg<i32> globalBeam = 6;
  util::Cfg<i32> rightBeam = 5;
  util::Cfg<i32> rightCheck = 1;
  util::Cfg<i32> logLevel = 0;
  util::Cfg<i32> autoStep = 0;
  util::Cfg<std::string> segmentSeparator{" "};

  void mergeWith(const JumanppConf& o) {
    configFile.mergeWith(o.configFile);
    modelFile.mergeWith(o.modelFile);
    outputType.mergeWith(o.outputType);
    outputFile.mergeWith(o.outputFile);
    inputType.mergeWith(o.inputType);
    inputFiles.mergeWith(o.inputFiles);
    rnnModelFile.mergeWith(o.rnnModelFile);
    rnnConfig.mergeWith(o.rnnConfig);
    graphvizDir.mergeWith(o.graphvizDir);
    beamSize.mergeWith(o.beamSize);
    beamOutput.mergeWith(o.beamOutput);
    globalBeam.mergeWith(o.globalBeam);
    rightBeam.mergeWith(o.rightBeam);
    rightCheck.mergeWith(o.rightCheck);
    logLevel.mergeWith(o.logLevel);
    autoStep.mergeWith(o.autoStep);
    segmentSeparator.mergeWith(o.segmentSeparator);
  }

  friend std::ostream& operator<<(std::ostream& os, const JumanppConf& conf);
};

Status parseCfgFile(StringPiece filename, JumanppConf* conf,
                    i32 recursionDepth = 0);
Status parseArgs(int argc, const char* argv[], JumanppConf* result);

}  // namespace jumandic
}  // namespace jumanpp

#endif  // JUMANPP_JUMANPP_ARGS_H
