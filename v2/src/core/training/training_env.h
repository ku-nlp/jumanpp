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

class TrainingEnv {
  const TrainingArguments& args_;
  core::JumanppEnv* env_;
  core::analysis::AnalyzerConfig aconf_;
  core::training::TrainingDataReader dataReader_;
  core::training::BatchedTrainer trainers_;
  core::training::SoftConfidenceWeighted scw_;
  core::training::TrainingExecutor executor_;

  StringPiece currentFilename_;
  util::MappedFile currentFile_;
  util::MappedFileFragment currentFragment_;

  float batchLoss_;
  float totalLoss_;

 public:
  TrainingEnv(const TrainingArguments& args, core::JumanppEnv* env)
      : args_{args}, env_{env}, scw_{args.trainingConfig} {
    aconf_.pageSize = 256 * 1024;
  }

  Status initialize();

  Status initFeatures(const core::features::StaticFeatureFactory* sff);

  Status loadInputData(StringPiece data);

  Status loadInput(StringPiece fileName);

  Status readOneBatch() { return trainers_.readBatch(&dataReader_); }

  Status handleProcessedTrainer(core::training::TrainingExecutionResult result,
                                float* curLoss);

  Status trainOneBatch();

  Status trainOneEpoch();

  std::unique_ptr<core::analysis::Analyzer> makeAnalyzer() const;
};

}  // namespace training
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_TRAINING_ENV_H
