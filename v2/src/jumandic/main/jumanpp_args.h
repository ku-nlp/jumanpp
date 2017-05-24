//
// Created by Arseny Tolmachev on 2017/03/11.
//

#ifndef JUMANPP_JUMANPP_ARGS_H
#define JUMANPP_JUMANPP_ARGS_H

#include <string>

namespace jumanpp {
namespace jumandic {

enum class OutputType { Juman, Morph };

struct JumanppConf {
  std::string modelFile;
  OutputType outputType = OutputType::Juman;
  std::string inputFile = "-";
  std::string rnnModelFile;
};

bool parseArgs(int argc, char* argv[], JumanppConf* result);

}  // namespace jumandic
}  // namespace jumanpp

#endif  // JUMANPP_JUMANPP_ARGS_H
