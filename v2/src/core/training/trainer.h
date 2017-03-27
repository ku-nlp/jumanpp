//
// Created by Arseny Tolmachev on 2017/03/27.
//

#ifndef JUMANPP_TRAINER_H
#define JUMANPP_TRAINER_H

#include "gold_example.h"
#include "loss.h"

namespace jumanpp {
namespace core {
namespace training {

class Trainer {
  analysis::AnalyzerImpl* analyzer;
  GoldenPath goldPath;
  TrainingExampleAdapter adapter;
};

}  // training
}  // core
}  // jumanpp

#endif  // JUMANPP_TRAINER_H
