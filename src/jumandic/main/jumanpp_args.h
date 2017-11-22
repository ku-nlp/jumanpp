//
// Created by Arseny Tolmachev on 2017/03/11.
//

#ifndef JUMANPP_JUMANPP_ARGS_H
#define JUMANPP_JUMANPP_ARGS_H

#include <string>
#include "core/analysis/rnn_scorer.h"
#include "core_config.h"

namespace jumanpp {
namespace jumandic {

enum class OutputType {
  Juman,
  Morph,
  FullMorph,
  DicSubset,
  Lattice,
#ifdef JPP_ENABLE_DEV_TOOLS
  GlobalBeamPos
#endif
};

struct JumanppConf {
  std::string modelFile;
  OutputType outputType = OutputType::Juman;
  std::string inputFile = "-";
  std::string rnnModelFile;
  core::analysis::rnn::RnnInferenceConfig rnnConfig{};
  std::string graphvizDir;
  i32 beamSize = 5;
  i32 beamOutput = 1;
  i32 globalBeam = -1;
  i32 rightBeam = -1;
  i32 rightCheck = -1;
  bool printVersion = false;
  bool printModelInfo = false;
};

bool parseArgs(int argc, char* argv[], JumanppConf* result);

}  // namespace jumandic
}  // namespace jumanpp

#endif  // JUMANPP_JUMANPP_ARGS_H
