//
// Created by Arseny Tolmachev on 2018/06/08.
//

#include "args.h"
#include "util/status.hpp"
#include "util/string_piece.h"

#include <iostream>

#ifdef __unix__
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#include <chrono>
#include "core/dic/progress.h"
#include "core/tool/index_cmd.h"
#include "core/tool/train_cmd.h"
#include "core/training/training_env.h"
#include "rnn/rnn_arg_parse.h"
#include "util/format.h"
#include "util/logging.hpp"

using namespace jumanpp;

template <typename T1, typename T2>
void copyValue(T1& out, args::ValueFlag<T2>& flag) {
  if (flag) {
    out = static_cast<const T1&>(flag.Get());
  }
}

template <typename T1>
void copyValue(T1& out, args::Base& flag, const T1& val) {
  if (flag.Matched()) {
    out = val;
  }
}

enum class ToolMode { Index, Train, EmbedRnn, StaticFeatures };

namespace t = ::jumanpp::core::training;

struct JumanppToolArgs {
  std::string specFile;
  std::string dictFile;
  std::string outModelFile;
  std::string comment;

  t::TrainingArguments trainArgs;

  ToolMode mode;

  static Status parseArgs(int argc, const char* argv[],
                          JumanppToolArgs* result) {
    args::ArgumentParser parser{"Juman++ Model Development Tool"};
    args::HelpFlag help{parser, "Help", "Show this message", {"help", 'h'}};
    args::Group commandGroup{parser, "Available commands"};
    args::Command index{commandGroup, "index",
                        "Index a raw dictionary into a seed model"};
    args::Command train{commandGroup, "train",
                        "Train a linear model weights using a seed model"};
    args::Command embedRnn{commandGroup, "embed-rnn",
                           "Embed a RNN into a trained model"};
    args::Command staticFeatures{commandGroup, "static-features",
                                 "Generate a C++ code for feature processing"};

    args::Group flagGroup{parser, "Parameters"};

    args::ValueFlag<std::string> specFile{
        flagGroup, "FILE", "Analysis Spec file", {"spec"}};
    args::ValueFlag<std::string> outModel{
        flagGroup, "FILE", "Output File", {"out-file"}};
    args::ValueFlag<std::string> comment{
        flagGroup, "STRING", "Comment to embed in model", {"comment"}};

    args::Group ioGroup{parser, "Input/Output"};
    args::ValueFlag<std::string> modelFile{
        ioGroup,
        "FILENAME",
        "Filename of preprocessed dictionary",
        {"model-input"}};
    args::ValueFlag<std::string> modelOutput{
        ioGroup,
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

    args::ValueFlag<std::string> scwDumpPrefix{
        ioGroup,
        "STRING",
        "Filename prefix for duming a SCW",
        {"scw-dump-prefix"},
        "scwdump"};
    args::ValueFlag<std::string> corpusComment{
        ioGroup,
        "COMMENT",
        "Comment to embed in SCW model about corpora",
        {"corpus-comment"},
        ""};

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
        trainingParams, "BEAM", "Node-local beam size, 5 default", {"beam"}, 5};
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
    args::ValueFlag<float> epsilon{trainingParams,
                                   "EPSILON",
                                   "stopping epsilon (1e-3)",
                                   {"epsilon"},
                                   1e-3f};

    args::Group gbeam{parser, "Boundary (Global) Beam Settings"};
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
    args::Flag firstIterFull{gbeam,
                             "Fill first iter",
                             "In each epoch, use full beam on first iteration",
                             {"gb-first-full"}};

    RnnArgs rnnArgs{parser};

    parser.helpParams.gutter = 4;
    parser.helpParams.helpindent = 35;
#if defined(TIOCGWINSZ)
    winsize winsz{0};
    auto sterr_no = fileno(stderr);
    if (ioctl(sterr_no, TIOCGWINSZ, &winsz) == 0 && isatty(sterr_no)) {
      parser.helpParams.width = winsz.ws_col;
      parser.helpParams.useColor = true;
    } else {
      parser.helpParams.width = 0xffff;
    }
#endif

    try {
      parser.ParseCLI(argc, argv);
    } catch (args::Help& h) {
      return JPPS_INVALID_PARAMETER << parser;
    } catch (std::exception& e) {
      return JPPS_INVALID_PARAMETER << e.what();
    }

    copyValue(result->mode, index, ToolMode::Index);
    copyValue(result->mode, train, ToolMode::Train);
    copyValue(result->mode, embedRnn, ToolMode::EmbedRnn);
    copyValue(result->mode, staticFeatures, ToolMode::StaticFeatures);

    copyValue(result->specFile, specFile);
    copyValue(result->outModelFile, outModel);
    copyValue(result->comment, comment);

    auto trg = &result->trainArgs;
    trg->trainingConfig.beamSize = beamSize.Get();
    trg->batchSize = batchSize.Get();
    trg->numThreads = numThreads.Get();
    trg->modelFilename = modelFile.Get();
    trg->outputFilename = modelOutput.Get();
    trg->corpusFilename = corpusFile.Get();
    trg->partialCorpus = partialCorpus.Get();
    trg->rnnModelFilename = rnnFile.Get();
    auto sizeExp = paramSizeExponent.Get();
    trg->trainingConfig.featureNumberExponent = sizeExp;
    trg->trainingConfig.randomSeed = randomSeed.Get();
    trg->trainingConfig.mode = trainMode.Get();
    trg->trainingConfig.scw.C = scwC.Get();
    trg->trainingConfig.scw.phi = scwPhi.Get();
    trg->batchMaxIterations = maxBatchIters.Get();
    trg->maxEpochs = maxEpochs.Get();
    trg->batchLossEpsilon = epsilon.Get();
    trg->rnnConfig = rnnArgs.config();
    trg->scwDumpDirectory = scwDumpDir.Get();
    trg->scwDumpPrefix = scwDumpPrefix.Get();
    trg->globalBeam.minLeftBeam = minLeftGbeam.Get();
    trg->globalBeam.maxLeftBeam = maxLeftGbeam.Get();
    trg->globalBeam.minRightBeam = minRightGbeam.Get();
    trg->globalBeam.maxRightBeam = maxRightGbeam.Get();
    trg->globalBeam.minRightCheck = minRcheckGbeam.Get();
    trg->globalBeam.maxRightCheck = maxRcheckGbeam.Get();
    trg->globalBeam.fullFirstIter = firstIterFull.Get();
    trg->comment = result->comment;

    return Status::Ok();
  }
};

struct StdoutProgressReporter : public core::ProgressCallback {
  std::string part;
  std::chrono::steady_clock::time_point lastUpdate =
      std::chrono::steady_clock::now();

  void report(u64 current, u64 total) override {
    if (current == total) {
      std::cout << "\r" << part << " done!" << std::endl;
      return;
    }

    auto now = std::chrono::steady_clock::now();
    auto timePassed = now - lastUpdate;
    if (timePassed < std::chrono::milliseconds(250)) {
      return;
    }

    lastUpdate = now;
    auto progress = static_cast<double>(current) / total;
    std::cout << "\r" << part
              << fmt::format(".. {0:.2f}%         ", progress * 100)
              << std::flush;
  }

  void recordName(StringPiece name) override { name.assignTo(part); }
};

void dieOnError(Status&& s) {
  if (!s) {
    std::cout << s;
    exit(1);
  }
}

void invokeTrain(const t::TrainingArguments& args) {
  int retval = core::tool::trainCommandImpl(args);
  exit(retval);
}

void invokeTool(const JumanppToolArgs& args) {
  switch (args.mode) {
    case ToolMode::Index: {
      core::tool::IndexTool tool;
      StdoutProgressReporter progress;
      tool.setProgressCallback(&progress);

      std::cout << "Indexing a dictionary!";
      std::cout << "\nSpec: " << args.specFile;
      std::cout << "\nRaw Dictionary: " << args.dictFile;
      std::cout << "\n";

      dieOnError(tool.indexDictionary(args.specFile, args.dictFile));
      std::cout << "\nSaving the model to: " << args.outModelFile;
      dieOnError(tool.saveModel(args.outModelFile, args.comment));
      std::cout << "\nSuccess!\n";
      break;
    }
    case ToolMode::Train:
      invokeTrain(args.trainArgs);
    default:
      std::cerr << "The tool is not implemented\n";
      exit(5);
  }
}

int main(int argc, const char* argv[]) {
  JumanppToolArgs args;
  Status s = JumanppToolArgs::parseArgs(argc, argv, &args);
  if (!s) {
    std::cerr << s << "\n";
    return 1;
  }

  invokeTool(args);

  return 0;
}