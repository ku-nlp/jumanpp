//
// Created by Arseny Tolmachev on 2017/03/09.
//

#include "jumanpp.h"
#include <fstream>
#include <iostream>
#include "jumanpp_args.h"
#include "util/logging.hpp"

using namespace jumanpp;

int main(int argc, const char** argv) {
  std::istream* inputSrc;
  std::unique_ptr<std::ifstream> filePtr;

  jumandic::JumanppConf conf;
  Status s = jumandic::parseArgs(argc, argv, &conf);
  if (!s) {
    std::cerr << s << "\n";
    return 1;
  }

  LOG_DEBUG() << "trying to create jumanppexec with model: "
              << conf.modelFile.value()
              << " and rnnmodel=" << conf.rnnModelFile.value();

  jumandic::JumanppExec exec{conf};
  s = exec.init();
  if (!s.isOk()) {
    if (conf.outputType == jumandic::OutputType::Version) {
      exec.printFullVersion();
      return 1;
    }

    if (conf.modelFile.isDefault()) {
      std::cerr << "Model file was not specified\n";
      return 1;
    }

    if (conf.outputType == jumandic::OutputType::ModelInfo) {
      exec.printModelInfo();
      return 1;
    }

    std::cerr << "failed to load model from disk: " << s;
    return 1;
  }

  if (conf.outputType == jumandic::OutputType::Version) {
    exec.printFullVersion();
    return 1;
  }

  if (conf.outputType == jumandic::OutputType::ModelInfo) {
    exec.printModelInfo();
    return 0;
  }

  if (conf.inputFile == "-") {
    inputSrc = &std::cin;
  } else {
    filePtr.reset(new std::ifstream{conf.inputFile});
    if (!*filePtr) {
      std::cerr << "could not open file " << conf.inputFile << " for reading";
      return 1;
    }
    inputSrc = filePtr.get();
  }

  std::string input;
  std::string comment;
  while (std::getline(*inputSrc, input)) {
    if (input.size() > 2 && input[0] == '#' && input[1] == ' ') {
      comment.clear();
      comment.append(input.begin() + 2, input.end());
      input.clear();
      std::getline(*inputSrc, input);
    }
    Status st = exec.analyze(input, comment);
    if (!st) {
      std::cerr << "error when analyzing sentence ";
      if (!comment.empty()) {
        std::cerr << "{" << comment << "} ";
      }
      std::cerr << "[ " << input << "]: " << st << "\n";
      std::cout << exec.emptyResult();
    } else {
      std::cout << exec.output();
    }
  }
  return 0;
}