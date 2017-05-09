//
// Created by Arseny Tolmachev on 2017/05/09.
//

#ifndef JUMANPP_JUMANPP_TRAIN_H
#define JUMANPP_JUMANPP_TRAIN_H

#include <string>

#include "core/training/training_types.h"

namespace jumanpp {
namespace jumandic {

struct JumanppTrainArgs {
  u32 beamSize;
  u32 batchSize;
  u32 numThreads;
  std::string modelFilename;
  std::string outputFilename;
  std::string corpusFilename;
  core::training::TrainingConfig trainingConfig;
};

}  // namespace jumandic
}  // namespace jumanpp

#endif  // JUMANPP_JUMANPP_TRAIN_H
