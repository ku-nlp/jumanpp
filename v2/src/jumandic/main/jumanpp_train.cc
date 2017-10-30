//
// Created by Arseny Tolmachev on 2017/05/09.
//

#include "jumanpp_train.h"
#include "args.h"
#include "core/training/training_env.h"
#include "jpp_rnn_args.h"
#include "jumandic/shared/jumandic_env.h"
#include "util/logging.hpp"

#include <iostream>

using namespace jumanpp;
namespace t = jumanpp::core::training;

Status parseArgs(int argc, const char** argv, t::TrainingArguments* args) {
  args::ArgumentParser parser{"Juman++ Training"};

  args::Group ioGroup{parser, "Input/Output"};
  args::ValueFlag<std::string> modelFile{ioGroup,
                                         "FILENAME",
                                         "Filename of preprocessed dictionary",
                                         {"model-input"}};
  args::ValueFlag<std::string> modelOutput{ioGroup,
                                           "FILENAME",
                                           "Model will be written to this file",
                                           {"model-output"}};
  args::ValueFlag<std::string> corpusFile{
      ioGroup,
      "FILENAME",
      "Filename of corpus that will be used for training",
      {"corpus"}};

  args::ValueFlag<std::string> partialCorpus{
      ioGroup,
      "FILENAME",
      "Filename of partially annotated corpus",
      {"partial-corpus"}};

  args::ValueFlag<std::string> rnnFile{
      ioGroup,
      "FILENAME",
      "Filename of fasterrnn trained model. It will be embedded inside the "
      "Juman++ model. RNN parameters will be embedded as well.",
      {"rnn-model"}};

  args::ValueFlag<std::string> scwDumpDir{
      ioGroup, "DIRECTORY", "Directory to dump SCW into", {"scw-dump-dir"}};

  args::ValueFlag<std::string> scwDumpPrefix{ioGroup,
                                             "STRING",
                                             "Filename prefix for duming a SCW",
                                             {"scw-dump-prefix"},
                                             "scwdump"};

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
  std::unordered_map<std::string, t::TrainingMode> tmodes{
      {"full", t::TrainingMode::Full},
      {"falloff", t::TrainingMode::FalloffBeam},
      {"violation", t::TrainingMode::MaxViolation}};

  args::MapFlag<std::string, t::TrainingMode> trainMode{
      trainingParams,    "MODE", "Training mode",
      {"training-mode"}, tmodes, t::TrainingMode::Full};
  t::ScwConfig scwCfg;
  args::ValueFlag<float> scwC{
      trainingParams, "VALUE", "SCW C parameter", {"scw-c"}, scwCfg.C};
  args::ValueFlag<float> scwPhi{
      trainingParams, "VALUE", "SCW phi parameter", {"scw-phi"}, scwCfg.phi};
  args::ValueFlag<u32> beamSize{
      trainingParams, "BEAM", "Beam size, 5 default", {"beam"}, 5};
  args::ValueFlag<u32> batchSize{
      trainingParams, "BATCH", "Batch Size, 1 default", {"batch"}, 1};
  args::ValueFlag<u32> numThreads{
      trainingParams, "THREADS", "# of threads, 1 default", {"threads"}, 1};
  args::ValueFlag<u32> maxBatchIters{trainingParams,
                                     "BATCH_ITERS",
                                     "max # of batch iterations",
                                     {"max-batch-iters"},
                                     1};
  args::ValueFlag<u32> maxEpochs{
      trainingParams, "EPOCHS", "max # of epochs (1)", {"max-epochs"}, 1};
  args::ValueFlag<float> epsilon{
      trainingParams, "EPSILON", "stopping epsilon (1e-3)", {"epsilon"}, 1e-3f};

  RnnArgs rnnArgs{parser};

  args::Group gbeam{parser, "Global Beam"};
  args::ValueFlag<i32> minLeftGbeam{
      gbeam, "VALUE", "Left Min", {"gb-left-min"}, -1};
  args::ValueFlag<i32> maxLeftGbeam{
      gbeam, "VALUE", "Left Max", {"gb-left-max"}, -1};
  args::ValueFlag<i32> minRightGbeam{
      gbeam, "VALUE", "Right Min", {"gb-right-min"}, -1};
  args::ValueFlag<i32> maxRightGbeam{
      gbeam, "VALUE", "Right Max", {"gb-right-max"}, -1};
  args::ValueFlag<i32> minRcheckGbeam{
      gbeam, "VALUE", "Right Check Min", {"gb-rcheck-min"}, -1};
  args::ValueFlag<i32> maxRcheckGbeam{
      gbeam, "VALUE", "Right Check Max", {"gb-rcheck-max"}, -1};

  try {
    parser.ParseCLI(argc, argv);
  } catch (args::ParseError& e) {
    std::cout << e.what() << "\n" << parser;
    std::exit(1);
  }

  args->trainingConfig.beamSize = beamSize.Get();
  args->batchSize = batchSize.Get();
  args->numThreads = numThreads.Get();
  args->modelFilename = modelFile.Get();
  args->outputFilename = modelOutput.Get();
  args->corpusFilename = corpusFile.Get();
  args->partialCorpus = partialCorpus.Get();
  args->rnnModelFilename = rnnFile.Get();
  auto sizeExp = paramSizeExponent.Get();
  args->trainingConfig.featureNumberExponent = sizeExp;
  args->trainingConfig.randomSeed = randomSeed.Get();
  args->trainingConfig.mode = trainMode.Get();
  args->trainingConfig.scw.C = scwC.Get();
  args->trainingConfig.scw.phi = scwPhi.Get();
  args->batchMaxIterations = maxBatchIters.Get();
  args->maxEpochs = maxEpochs.Get();
  args->batchLossEpsilon = epsilon.Get();
  args->rnnConfig = rnnArgs.config();
  args->scwDumpDirectory = scwDumpDir.Get();
  args->scwDumpPrefix = scwDumpPrefix.Get();
  args->globalBeam.minLeftBeam = minLeftGbeam.Get();
  args->globalBeam.maxLeftBeam = maxLeftGbeam.Get();
  args->globalBeam.minRightBeam = minRightGbeam.Get();
  args->globalBeam.maxRightBeam = maxRightGbeam.Get();
  args->globalBeam.minRightCheck = minRcheckGbeam.Get();
  args->globalBeam.maxRightCheck = maxRcheckGbeam.Get();

  if (sizeExp > 31) {
    return Status::InvalidState() << "size exponent was too large: " << sizeExp
                                  << ", maximum allowed is 31";
  }

  if (args->modelFilename.empty()) {
    std::cout << "Model filename was not specified\n";
    std::cout << parser;
    std::exit(1);
  }

  return Status::Ok();
}

void doTrain(core::training::TrainingEnv& env,
             const t::TrainingArguments& args) {
  float lastLoss = 0.0f;

  LOG_INFO() << "Starting SCW model training...";

  for (int nepoch = 0; nepoch < args.maxEpochs; ++nepoch) {
    env.resetInput();
    float ratio = static_cast<float>(nepoch) / args.maxEpochs;
    env.changeGlobalBeam(1.0f - ratio);

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

int doTrainJpp(t::TrainingArguments& args, core::JumanppEnv& env) {
  env.setBeamSize(args.trainingConfig.beamSize);

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
  exec.exportScwParams(&model);

  return saveModel(args, model);
}

int doEmbedRnn(t::TrainingArguments& args, core::JumanppEnv& env) {
  LOG_INFO() << "embedding the rnn into the model file";

  rnn::mikolov::MikolovModelReader rnnReader;
  Status s = rnnReader.open(args.rnnModelFilename);
  if (!s) {
    LOG_ERROR() << "failed to open rnn file: " << args.rnnModelFilename << "\n"
                << s;
    return 1;
  }

  s = rnnReader.parse();
  if (!s) {
    LOG_ERROR() << "failed to parse rnn model from file: "
                << args.rnnModelFilename << "\n"
                << s;
    return 1;
  }

  core::analysis::rnn::RnnHolder rnnHolder;
  s = rnnHolder.init(args.rnnConfig, rnnReader, env.coreHolder()->dic(),
                     "surface");
  if (!s) {
    LOG_ERROR() << "failed to initialize rnn: " << s;
    return 1;
  }

  auto info = env.modelInfoCopy();

  s = rnnHolder.makeInfo(&info);
  if (!s) {
    LOG_ERROR() << "failed to add rnn info to the model: " << s;
    return 1;
  }

  return saveModel(args, info);
}

int main(int argc, const char** argv) {
  t::TrainingArguments args{};
  Status s = parseArgs(argc, argv, &args);
  if (!s) {
    LOG_ERROR() << "failed to parse arguments: " << s;
    return 1;
  }

  core::JumanppEnv env;

  s = env.loadModel(args.modelFilename);
  if (!s) {
    LOG_ERROR() << "failed to read model from disk: " << s;
    return 1;
  }

  if (args.rnnModelFilename.empty()) {
    return doTrainJpp(args, env);
  }

  return doEmbedRnn(args, env);
}