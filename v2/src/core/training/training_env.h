//
// Created by Arseny Tolmachev on 2017/05/10.
//

#ifndef JUMANPP_TRAINING_ENV_H
#define JUMANPP_TRAINING_ENV_H

#include "core/training/scw.h"
#include "core/training/trainer.h"
#include "core/training/training_executor.h"

namespace jumanpp {
namespace core {

class JumanppEnv;

namespace training {

struct TrainingArguments {
  u32 batchSize = 1;
  u32 numThreads = 1;
  u32 batchMaxIterations = 1;
  u32 maxEpochs = 1;
  float batchLossEpsilon = 1e-3f;
  std::string modelFilename;
  std::string outputFilename;
  std::string corpusFilename;
  TrainingConfig trainingConfig;
};

class TrainingEnv {
  const TrainingArguments& args_;
  core::JumanppEnv* env_;
  core::analysis::AnalyzerConfig aconf_;
  TrainingDataReader dataReader_;
  BatchedTrainer trainers_;
  SoftConfidenceWeighted scw_;
  TrainingExecutor executor_;

  StringPiece currentFilename_;
  util::FullyMappedFile currentFile_;

  float batchLoss_;
  float totalLoss_;

 public:
  TrainingEnv(const TrainingArguments& args, JumanppEnv* env)
      : args_{args}, env_{env}, scw_{args.trainingConfig} {
    aconf_.pageSize = 256 * 1024;
  }

  void warnOnNonMatchingFeatures(const spec::TrainingSpec& spec);

  Status initFeatures(const core::features::StaticFeatureFactory* sff);

  Status initOther();

  Status loadInputData(StringPiece data);

  Status loadInput(StringPiece fileName);
  void resetInput();

  Status readOneBatch() { return trainers_.readBatch(&dataReader_); }

  Status handleProcessedTrainer(TrainingExecutionResult result, float* curLoss);

  Status trainOneBatch();

  float batchLoss() const { return batchLoss_; }
  float epochLoss() const { return totalLoss_; }

  Status trainOneEpoch();

  i32 numTrainers() const { return trainers_.activeTrainers(); }
  OwningTrainer* trainer(i32 idx) { return trainers_.trainer(idx); }

  std::unique_ptr<analysis::Analyzer> makeAnalyzer(i32 beamSize = -1) const;

  void exportScwParams(model::ModelInfo* pInfo);
};

}  // namespace training
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_TRAINING_ENV_H
