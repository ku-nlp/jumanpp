//
// Created by Arseny Tolmachev on 2017/03/11.
//

#ifndef JUMANPP_JUMANPP_ARGS_H
#define JUMANPP_JUMANPP_ARGS_H

#include <string>
#include "core/analysis/rnn_scorer.h"

namespace jumanpp {
namespace jumandic {

enum class OutputType { Juman, Morph, DicSubset, Lattice };

struct JumanppConf {
  std::string modelFile;
  OutputType outputType = OutputType::Juman;
  std::string inputFile = "-";
  std::string rnnModelFile;
  core::analysis::rnn::RnnInferenceConfig rnnConfig{};
  std::string graphvizDir;
  i32 beamSize = 5;
  i32 beamOutput = 1;
};

bool parseArgs(int argc, char* argv[], JumanppConf* result);

}  // namespace jumandic
}  // namespace jumanpp

#endif  // JUMANPP_JUMANPP_ARGS_H
