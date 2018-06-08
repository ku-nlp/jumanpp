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
#include "index_cmd.h"
#include "util/format.h"

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

struct JumanppToolArgs {
  std::string specFile;
  std::string dictFile;
  std::string inModelFile;
  std::string outModelFile;
  std::string comment;

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