//
// Created by Arseny Tolmachev on 2017/05/09.
//

#ifndef JUMANPP_JUMANPP_TRAIN_H
#define JUMANPP_JUMANPP_TRAIN_H

#include <string>

#include "core/training/training_types.h"

namespace jumanpp {
namespace jumandic {

struct TrainingArguments {
  u32 beamSize = 5;
  u32 batchSize = 1;
  u32 numThreads = 1;
  u32 batchMaxIterations = 1;
  u32 maxEpochs = 1;
  float batchLossEpsilon = 1e-3f;
  std::string modelFilename;
  std::string outputFilename;
  std::string corpusFilename;
  core::training::TrainingConfig trainingConfig;
};

}  // namespace jumandic
}  // namespace jumanpp

#endif  // JUMANPP_JUMANPP_TRAIN_H
