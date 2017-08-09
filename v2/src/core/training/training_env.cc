//
// Created by Arseny Tolmachev on 2017/05/10.
//

#include "training_env.h"
#include "core/env.h"
#include "util/debug_output.h"
#include "util/logging.hpp"

namespace jumanpp {
namespace core {
namespace training {

Status TrainingEnv::trainOneEpoch() {
  double lastLoss = 0.f;
  double lossSum = 0.0f;
  while (!dataReader_.finished()) {
    JPP_RETURN_IF_ERROR(readOneBatch());
    if (trainers_.activeTrainers() <= 0) {
      break;
    }

    for (u32 batchIter = 0; batchIter < args_.batchMaxIterations; ++batchIter) {
      JPP_RETURN_IF_ERROR(trainOneBatch());
      auto normLoss =
          std::abs(lastLoss - batchLoss_) / trainers_.activeTrainers();
      lastLoss = batchLoss_;
      auto firstTrainer = trainers_.trainer(0)->line();
      auto lastTrainer = firstTrainer + trainers_.activeTrainers() - 1;
      LOG_DEBUG() << "batch [" << firstTrainer << "-" << lastTrainer << "]|"
                  << batchIter << ": " << normLoss << "|" << batchLoss_;
      if (normLoss < args_.batchLossEpsilon) {
        break;
      }
    }
    lossSum += lastLoss;
  }
  totalLoss_ = lossSum;
  return Status::Ok();
}

Status TrainingEnv::trainOneBatch() {
  double curLoss = 0;

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
    core::training::TrainingExecutionResult result, double* curLoss) {
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

void TrainingEnv::resetInput() {
  dataReader_.resetInput(currentFile_.contents());
  batchLoss_ = 0;
  totalLoss_ = 0;
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
    const core::features::StaticFeatureFactory* sff) {
  return env_->initFeatures(sff);
}

Status TrainingEnv::initOther() {
  auto pHolder = env_->coreHolder();
  if (pHolder == nullptr) {
    return Status::InvalidState() << "core holder was not constructed yet";
  }
  auto& trainingSpec = env_->spec().training;  // make a copy here
  warnOnNonMatchingFeatures(trainingSpec);

  JPP_RETURN_IF_ERROR(dataReader_.initialize(trainingSpec, *pHolder));
  core::training::TrainerFullConfig conf{aconf_, *pHolder, trainingSpec,
                                         args_.trainingConfig};
  auto sconf = scw_.scorers();
  JPP_RETURN_IF_ERROR(trainers_.initialize(conf, sconf, args_.batchSize));
  JPP_RETURN_IF_ERROR(executor_.initialize(sconf, args_.numThreads));
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

void TrainingEnv::exportScwParams(model::ModelInfo* pInfo) {
  scw_.exportModel(pInfo);
}

void TrainingEnv::warnOnNonMatchingFeatures(const spec::TrainingSpec& spec) {
  analysis::LatticeCompactor lcm{env_->coreHolder()->dic().entries()};
  lcm.initialize(nullptr, env_->coreHolder()->runtime());
  std::vector<i32> fields;
  std::vector<i32> diff1;
  std::vector<i32> diff2;

  auto usedFeatures = lcm.usedFeatures();

  for (auto& x : spec.fields) {
    if (x.weight != 0) {
      fields.push_back(x.fieldIdx);
    }
  }

  util::sort(fields);

  std::set_difference(usedFeatures.begin(), usedFeatures.end(), fields.begin(),
                      fields.end(), std::back_inserter(diff1));
  std::set_difference(fields.begin(), fields.end(), usedFeatures.begin(),
                      usedFeatures.end(), std::back_inserter(diff2));

  if (diff1.size() != 0 || diff2.size() != 0) {
    LOG_WARN() << "fields that are used for features are not equal to gold "
                  "fields: features="
               << VOut(diff1) << " gold=" << VOut(diff2);
  }
}

}  // namespace training
}  // namespace core
}  // namespace jumanpp
