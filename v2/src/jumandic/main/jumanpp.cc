//
// Created by Arseny Tolmachev on 2017/03/09.
//

#include <iostream>
#include "jumanpp.h"

using namespace jumanpp;


int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "must pass model file as a second argument\n";
    return 1;
  }

  JumanppExec exec;
  StringPiece modelName{argv[1]};
  Status s = exec.init(modelName);
  if (!s.isOk()) {
    std::cerr << "failed to load model from disk: " << s.message;
    return 0;
  }

  std::string input;
  while (std::cin) {
    std::getline(std::cin, input);
    Status st = exec.analyze(input);
    if (!st) {
      std::cerr << "error when analyzing sentence: " << st.message;
      return 1;
    } else {
      std::cout << exec.output();
    }
  }
}