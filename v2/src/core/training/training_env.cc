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
  i32 firstTrainer = 0;
  while (!fullReader_.finished()) {
    JPP_RETURN_IF_ERROR(readOneBatch());
    prepareTrainers();
    if (trainers_.totalTrainers() <= 0) {
      break;
    }
    auto lastTrainer = firstTrainer + trainers_.totalTrainers() - 1;

    for (u32 batchIter = 0; batchIter < args_.batchMaxIterations; ++batchIter) {
      JPP_RETURN_IF_ERROR(trainOneBatch());
      auto normLoss =
          std::abs(lastLoss - batchLoss_) / trainers_.totalTrainers();
      lastLoss = batchLoss_;
      LOG_DEBUG() << "batch [" << firstTrainer << "-" << lastTrainer << "]|"
                  << batchIter << ": " << normLoss << "|" << batchLoss_;
      if (normLoss < args_.batchLossEpsilon) {
        break;
      }
    }
    lossSum += lastLoss;
    firstTrainer += trainers_.totalTrainers();
  }

  if (firstEpoch_) {
    auto zeroed = scw_.substractInitValues();
    auto total = scw_.numWeights();
    auto ratio = float(zeroed) / total;
    LOG_DEBUG() << "Zeroed " << zeroed << " features of " << total << " ("
                << ratio * 100 << "%)";
    firstEpoch_ = false;
  }

  totalLoss_ = lossSum;
  return Status::Ok();
}

Status TrainingEnv::trainOneBatch() {
  double curLoss = 0;

  auto totalTrainers = trainers_.totalTrainers();
  int submittedTrainers = 0;

  for (int processedTrainers = 0; processedTrainers < totalTrainers;
       ++processedTrainers) {
    // step1: load trainer queue with trainers
    while (submittedTrainers < totalTrainers &&
           executor_.submitNext(trainers_.trainer(submittedTrainers))) {
      submittedTrainers += 1;
    }

    // step2: wait for ready trainers and process them
    JPP_RETURN_IF_ERROR(handleProcessedTrainer(executor_.waitOne(), &curLoss));
  }

  JPP_DCHECK_EQ(executor_.nowReady(), 0);

  batchLoss_ = curLoss;
  return Status::Ok();
}

Status TrainingEnv::handleProcessedTrainer(
    core::training::TrainingExecutionResult result, double* curLoss) {
  auto processed = result.trainer;
  if (!result.processStatus) {
    i64 lineNo = -1;
    if (processed != nullptr) {
      lineNo = result.trainer->exampleInfo().line;
    }
    JPP_RIE_MSG(std::move(result.processStatus),
                "failed to process example on line #" << lineNo);
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

  JPP_RETURN_IF_ERROR(loadInputData(data));
  fullReader_.setFilename(fileName);
  return Status::Ok();
}

void TrainingEnv::resetInput() {
  fullReader_.resetInput(currentFile_.contents());
  batchLoss_ = 0;
  totalLoss_ = 0;
}

Status TrainingEnv::loadInputData(StringPiece data) {
  auto format = this->args_.trainingConfig.inputFormat;
  if (format == InputFormat::Csv) {
    this->fullReader_.initCsv(data);
  } else if (format == InputFormat::Morph) {
    this->fullReader_.initDoubleCsv(data);
  } else {
    return Status::InvalidState() << "unsupported input format";
  }
  fullReader_.setFilename("<memory>");

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

  JPP_RETURN_IF_ERROR(trainingIo_.initialize(trainingSpec, *pHolder));
  fullReader_.setTrainingIo(&trainingIo_);
  core::training::TrainerFullConfig conf{&aconf_, pHolder, &trainingSpec,
                                         &args_.trainingConfig};
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

Status TrainingEnv::loadPartialExamples(StringPiece filename) {
  JPP_RETURN_IF_ERROR(partReader_.initialize(&trainingIo_));
  JPP_RETURN_IF_ERROR(partReader_.openFile(filename))
  JPP_RETURN_IF_ERROR(trainers_.readPartialExamples(&partReader_));
  return Status::Ok();
}

}  // namespace training
}  // namespace core
}  // namespace jumanpp
