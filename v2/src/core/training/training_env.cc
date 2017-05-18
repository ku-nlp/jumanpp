//
// Created by Arseny Tolmachev on 2017/05/10.
//

#include "training_env.h"
#include "core/env.h"
#include "util/logging.hpp"

namespace jumanpp {
namespace core {
namespace training {

Status TrainingEnv::trainOneEpoch() {
  float lossSum = 0;
  while (!dataReader_.finished()) {
    JPP_RETURN_IF_ERROR(readOneBatch());
    if (trainers_.activeTrainers() <= 0) {
      break;
    }
    float lastLoss = -100000000.f;
    for (u32 batchIter = 0; batchIter < args_.batchMaxIterations; ++batchIter) {
      JPP_RETURN_IF_ERROR(trainOneBatch());
      lossSum += batchLoss_;

      auto normLoss =
          std::abs(lastLoss - batchLoss_) / trainers_.activeTrainers();
      auto firstTrainer = trainers_.trainer(0)->line();
      auto lastTrainer = firstTrainer + trainers_.activeTrainers();
      LOG_DEBUG() << "batch [" << firstTrainer << "-" << lastTrainer << "]|"
                  << batchIter << ": " << normLoss << "|" << batchLoss_;
      if (normLoss < args_.batchLossEpsilon) {
        break;
      }
    }
  }
  totalLoss_ = lossSum;
  return Status::Ok();
}

Status TrainingEnv::trainOneBatch() {
  float curLoss = 0;

  core::training::TrainingExecutionResult result{nullptr,
                                                 Status::NotImplemented()};

  for (int i = 0; i < trainers_.activeTrainers(); ++i) {
    auto example = trainers_.trainer(i);
    if (executor_.runNext(example, &result)) {
      JPP_RETURN_IF_ERROR(handleProcessedTrainer(result, &curLoss));
    }
  }

  // handle currently processing elements
  while (executor_.nonProcessedExist()) {
    JPP_RETURN_IF_ERROR(handleProcessedTrainer(executor_.waitOne(), &curLoss));
  }

  batchLoss_ = curLoss;
  return Status::Ok();
}

Status TrainingEnv::handleProcessedTrainer(
    core::training::TrainingExecutionResult result, float *curLoss) {
  auto processed = result.trainer;
  if (!result.processStatus) {
    i64 lineNo = -1;
    if (processed != nullptr) {
      lineNo = processed->line();
    }
    return Status::InvalidState()
           << "failed to process example on line #" << lineNo << ": "
           << result.processStatus.message;
  }
  auto loss = processed->loss();
  *curLoss += loss;
  scw_.update(loss, processed->featureDiff());
  return Status::Ok();
}

Status TrainingEnv::loadInput(StringPiece fileName) {
  JPP_RETURN_IF_ERROR(currentFile_.open(fileName));

  auto data = currentFile_.contents();

  batchLoss_ = 0;
  totalLoss_ = 0;

  return loadInputData(data);
}

Status TrainingEnv::loadInputData(StringPiece data) {
  auto format = this->args_.trainingConfig.inputFormat;
  if (format == InputFormat::Csv) {
    this->dataReader_.initCsv(data);
  } else if (format == InputFormat::Morph) {
    this->dataReader_.initDoubleCsv(data);
  } else {
    return Status::InvalidState() << "unsupported input format";
  }

  return Status::Ok();
}

Status TrainingEnv::initFeatures(
    const core::features::StaticFeatureFactory *sff) {
  return env_->initFeatures(sff);
}

Status TrainingEnv::initialize() {
  auto pHolder = env_->coreHolder();
  if (pHolder == nullptr) {
    return Status::InvalidState() << "core holder was not constructed yet";
  }
  JPP_RETURN_IF_ERROR(dataReader_.initialize(env_->spec().training, *pHolder));
  core::training::TrainerFullConfig conf{
      aconf_, *pHolder, env_->spec().training, args_.trainingConfig};
  auto sconf = scw_.scorers();
  trainers_.initialize(conf, sconf, args_.batchSize);
  executor_.initialize(sconf, args_.numThreads);
  return Status::Ok();
}

std::unique_ptr<analysis::Analyzer> TrainingEnv::makeAnalyzer(
    i32 beamSize) const {
  if (beamSize == -1) {
    beamSize = args_.trainingConfig.beamSize;
  }
  ScoringConfig sconf{beamSize, 1};
  auto ptr = std::unique_ptr<analysis::Analyzer>(new analysis::Analyzer{});
  if (!ptr->initialize(env_->coreHolder(), aconf_, sconf, scw_.scorers())) {
    ptr.reset();
  }
  return ptr;
}

}  // namespace training
}  // namespace core
}  // namespace jumanpp
