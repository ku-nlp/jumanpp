//
// Created by Arseny Tolmachev on 2017/03/11.
//

#include "jumanpp_args.h"
#include <iostream>
#include "args.h"

namespace jumanpp {
namespace jumandic {

bool parseArgs(int argc, char* argv[], JumanppConf* result) {
  args::ArgumentParser parser{"Juman++ v2"};

  args::Group outputType{parser, "Output type"};
  args::Flag juman{outputType, "juman", "Juman", {'j', "juman"}};

  args::Group modelParams{parser, "Model parameters"};
  args::ValueFlag<std::string> modelFile{
      modelParams, "model", "Model filename", {"model"}};
  args::ValueFlag<std::string> rnnModelFile{
      modelParams, "rnn model", "RNN model filename", {"rnn-model"}};

  args::Positional<std::string> input{parser, "input",
                                      "Input filename (- for stdin)", "-"};

  if (result == nullptr) {
    std::cerr << parser;
    exit(1);
  }

  try {
    parser.ParseCLI(argc, argv);
  } catch (args::Help&) {
    std::cerr << parser;
    exit(1);
  } catch (args::ParseError& e) {
    std::cerr << e.what() << "\n";
    std::cerr << parser;
    exit(1);
  } catch (...) {
    return false;
  }

  result->outputType = OutputType::Juman;
  if (juman) {
    result->outputType = OutputType::Juman;
  }

  result->inputFile = input.Get();

  if (!modelFile) {
    std::cerr << "model file is not specified\n";
    return false;
  }
  result->modelFile = modelFile.Get();
  result->rnnModelFile = rnnModelFile.Get();

  return true;
}

}  // jumandic
}  // jumanpp