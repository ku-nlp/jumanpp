//
// Created by Arseny Tolmachev on 2017/05/09.
//

#include "jumanpp_train.h"
#include "args.h"
#include "core/training/scw.h"
#include "core/training/trainer.h"
#include "core/training/training_executor.h"
#include "jumandic/shared/jumandic_env.h"
#include "util/logging.hpp"
#include "util/status.hpp"

#include <iostream>

using namespace jumanpp;

Status parseArgs(int argc, const char** argv,
                 jumanpp::jumandic::TrainingArguments* args) {
  args::ArgumentParser parser{"Juman++ Training"};

  args::Group ioGroup{parser, "Input/Output"};
  args::ValueFlag<std::string> modelFile{ioGroup,
                                         "modelInFile",
                                         "Filename of preprocessed dictionary",
                                         {"model-input"}};
  args::ValueFlag<std::string> modelOutput{ioGroup,
                                           "modelOutFile",
                                           "Model will be written to this file",
                                           {"model-output"}};
  args::ValueFlag<std::string> corpusFile{
      ioGroup,
      "corpus",
      "Filename of corpus that will be used for training",
      {"corpus"}};

  args::Group trainingParams{parser, "Training parameters"};
  args::ValueFlag<u32> paramSizeExponent{
      trainingParams,
      "SIZE",
      "Param size will be 2^SIZE, 15 (32k) default",
      {"size"},
      15};
  args::ValueFlag<u32> randomSeed{trainingParams,
                                  "SEED",
                                  "RNG seed, 0xdeadbeef default",
                                  {"seed"},
                                  0xdeadbeefU};
  args::ValueFlag<u32> beamSize{
      trainingParams, "BEAM", "Beam size, 5 default", {"beam"}, 5};
  args::ValueFlag<u32> batchSize{
      trainingParams, "BATCH", "Batch Size, 1 default", {"batch"}, 1};
  args::ValueFlag<u32> numThreads{
      trainingParams, "THREADS", "# of threads, 1 default", {"threads"}, 1};

  try {
    parser.ParseCLI(argc, argv);
  } catch (args::ParseError& e) {
    std::cout << e.what() << "\n" << parser;
    std::exit(1);
  }

  args->beamSize = beamSize.Get();
  args->batchSize = batchSize.Get();
  args->numThreads = numThreads.Get();
  args->modelFilename = modelFile.Get();
  args->outputFilename = modelOutput.Get();
  args->corpusFilename = corpusFile.Get();
  auto sizeExp = paramSizeExponent.Get();
  args->trainingConfig.featureNumberExponent = sizeExp;
  args->trainingConfig.randomSeed = randomSeed.Get();

  if (sizeExp > 31) {
    return Status::InvalidState() << "size exponent was too large: " << sizeExp
                                  << ", maximum allowed is 31";
  }

  return Status::Ok();
}

class JumandicTrainingExec {
  const jumandic::TrainingArguments& args_;
  jumandic::JumandicEnv* env_;
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
  JumandicTrainingExec(const jumandic::TrainingArguments& args,
                       jumandic::JumandicEnv* env)
      : args_{args}, env_{env}, scw_{args.trainingConfig} {
    aconf_.pageSize = 256 * 1024;
  }

  Status initialize() {
    auto pHolder = env_->coreHolder();
    if (pHolder == nullptr) {
      return Status::InvalidState() << "core holder was not constructed yet";
    }
    JPP_RETURN_IF_ERROR(
        dataReader_.initialize(env_->spec().training, *pHolder));
    core::training::TrainerFullConfig conf{
        aconf_, *pHolder, env_->spec().training, args_.trainingConfig};
    trainers_.initialize(conf, args_.batchSize);
    executor_.initialize(scw_.scoreConfig(), args_.numThreads);
    return Status::Ok();
  }

  Status initFeatures(const core::features::StaticFeatureFactory* sff) {
    return env_->initFeatures(sff);
  }

  Status loadInputData(StringPiece data) {
    auto format = this->args_.trainingConfig.inputFormat;
    if (format == core::training::InputFormat::Csv) {
      this->dataReader_.initCsv(data);
    } else if (format == core::training::InputFormat::Morph) {
      this->dataReader_.initDoubleCsv(data);
    } else {
      return Status::InvalidState() << "unsupported input format";
    }

    return Status::Ok();
  }

  Status loadInput(StringPiece fileName) {
    if (currentFilename_ != fileName) {
      JPP_RETURN_IF_ERROR(
          currentFile_.open(fileName, util::MMapType::ReadOnly));
      JPP_RETURN_IF_ERROR(
          currentFile_.map(&currentFragment_, 0, currentFile_.size()));
    }

    auto data = currentFragment_.asStringPiece();

    batchLoss_ = 0;
    totalLoss_ = 0;

    return loadInputData(data);
  }

  Status readOneBatch() { return trainers_.readBatch(&dataReader_); }

  Status handleProcessedTrainer(core::training::TrainingExecutionResult result,
                                float* curLoss) {
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

  Status trainOneBatch() {
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
      JPP_RETURN_IF_ERROR(
          handleProcessedTrainer(executor_.waitOne(), &curLoss));
    }

    batchLoss_ = curLoss;
    return Status::Ok();
  }

  Status trainOneEpoch() {
    while (!dataReader_.finished()) {
      JPP_RETURN_IF_ERROR(readOneBatch());
      float lastLoss = -100000000.f;
      float lossSum = 0;
      for (u32 batchIter = 0; batchIter < args_.batchMaxIterations;
           ++batchIter) {
        JPP_RETURN_IF_ERROR(trainOneBatch());
        lossSum += batchLoss_;

        auto normLoss =
            std::abs(lastLoss - batchLoss_) / trainers_.activeTrainers();
        auto firstTrainer = trainers_.trainer(0)->line();
        auto lastTrainer = firstTrainer + trainers_.activeTrainers();
        LOG_DEBUG() << "batch [" << firstTrainer << "-" << lastTrainer << "]|"
                    << batchIter << ": " << normLoss;
        if (normLoss < args_.batchLossEpsilon) {
          break;
        }
      }
    }
  }
};

int main(int argc, const char** argv) {
  jumandic::TrainingArguments args{};
  Status s = parseArgs(argc, argv, &args);
  if (!s) {
    LOG_ERROR() << "failed to parse arguments: " << s.message;
    return 1;
  }

  jumandic::JumandicEnv env;

  s = env.loadModel(args.modelFilename);
  if (!s) {
    LOG_ERROR() << "failed to read model from disk: " << s.message;
    return 1;
  }
  env.setBeamSize(args.beamSize);

  JumandicTrainingExec exec{args, &env};
  s = exec.initialize();

  if (!s) {
    LOG_ERROR() << "failed to initialize training process: " << s.message;
  }

  return 0;
}