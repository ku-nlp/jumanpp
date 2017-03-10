//
// Created by Arseny Tolmachev on 2017/03/09.
//

#include <iostream>
#include <fstream>
#include "jumanpp.h"

using namespace jumanpp;


int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "must pass model file as a second argument\n";
    return 1;
  }

  std::istream* inputSrc;
  std::unique_ptr<std::ifstream> filePtr;

  if (argc == 3) {
    auto inputFile = argv[2];
    filePtr.reset(new std::ifstream{inputFile, std::ios::in});
    if (!*filePtr) {
      std::cerr << "could not open input file " << inputFile << "\n";
    }
    inputSrc = filePtr.get();
  } else {
    inputSrc = &std::cin;
  }

  JumanppExec exec;
  StringPiece modelName{argv[1]};
  Status s = exec.init(modelName);
  if (!s.isOk()) {
    std::cerr << "failed to load model from disk: " << s.message;
    return 1;
  }

  std::string input;
  while (*inputSrc) {
    std::getline(*inputSrc, input);
    Status st = exec.analyze(input);
    if (!st) {
      std::cerr << "error when analyzing sentence [ " << input << "] :" << st.message << "\n";
      std::cout << "EOS\n";
    } else {
      std::cout << exec.output();
    }
  }
  return 0;
}