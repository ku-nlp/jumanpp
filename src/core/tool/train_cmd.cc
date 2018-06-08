//
// Created by Arseny Tolmachev on 2018/06/08.
//

#include "train_cmd.h"
#include "core/env.h"
#include "util/logging.hpp"

namespace jumanpp {
namespace core {
namespace tool {

namespace {

namespace t = ::jumanpp::core::training;

void doTrain(core::training::TrainingEnv& env,
             const t::TrainingArguments& args) {
  float lastLoss = 0.0f;

  LOG_INFO() << "Starting SCW model training...";

  for (int nepoch = 0; nepoch < args.maxEpochs; ++nepoch) {
    env.resetInput();
    float ratio = static_cast<float>(nepoch) / args.maxEpochs;
    env.changeGlobalBeam(ratio);

    LOG_INFO() << "Starting E#" << nepoch
               << " Global beam: " << env.globalBeam();
    Status s = env.trainOneEpoch();
    if (!s) {
      LOG_ERROR() << "failed to train: " << s;
      exit(1);
    }

    LOG_INFO() << "E#" << nepoch << " finished, loss=" << env.epochLoss();

    if (!args.scwDumpDirectory.empty()) {
      env.dumpScw(args.scwDumpDirectory, args.scwDumpPrefix, nepoch);
    }

    if (std::abs(lastLoss - env.epochLoss()) < args.batchLossEpsilon) {
      return;
    }
  }
}

int saveModel(const core::training::TrainingArguments& args,
              const core::model::ModelInfo& model) {
  core::model::ModelSaver saver;
  Status s = saver.open(args.outputFilename);
  if (!s) {
    LOG_ERROR() << "failed to open file [" << args.outputFilename
                << "] for saving model: " << s;
    return 1;
  }

  LOG_INFO() << "Writing the model to file: " << args.outputFilename;
  s = saver.save(model);
  if (!s) {
    LOG_ERROR() << "failed to save model: " << s;
  }
  LOG_INFO() << "Model saved successfully";

  return 0;
}

int doTrainJpp(const t::TrainingArguments& args, core::JumanppEnv& env) {
  env.setBeamSize(static_cast<u32>(args.trainingConfig.beamSize));

  t::TrainingEnv exec{args, &env};

  Status s = exec.initFeatures(nullptr);

  if (!s) {
    LOG_ERROR() << "failed to initialize features: " << s;
    return 1;
  }

  s = exec.initOther();

  if (!s) {
    LOG_ERROR() << "failed to initialize training process: " << s;
    return 1;
  }

  if (!args.partialCorpus.empty()) {
    s = exec.loadPartialExamples(args.partialCorpus);
    if (!s) {
      LOG_ERROR() << "failed to load partially annotated corpus: " << s;
    }
  }

  s = exec.loadInput(args.corpusFilename);
  if (!s) {
    LOG_ERROR() << "failed to open corpus filename: " << s;
    return 1;
  }

  doTrain(exec, args);

  auto model = env.modelInfoCopy();
  exec.exportScwParams(&model, args.comment);

  return saveModel(args, model);
}

}  // namespace

int trainCommandImpl(const training::TrainingArguments& args) {
  core::JumanppEnv env;

  Status s = env.loadModel(args.modelFilename);
  if (!s) {
    LOG_ERROR() << "failed to read model from disk: " << s;
    return 1;
  }

  if (args.rnnModelFilename.empty()) {
    return doTrainJpp(args, env);
  }

  LOG_ERROR() << "Model filename is empty";
  return 1;
}

}  // namespace tool
}  // namespace core
}  // namespace jumanpp