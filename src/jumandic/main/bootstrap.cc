//
// Created by Arseny Tolmachev on 2017/03/08.
//

#include <iostream>
#include "args.h"
#include "core/dic/dic_builder.h"
#include "core/dic/dictionary.h"
#include "core/dic/progress.h"
#include "core/impl/model_io.h"
#include "jumandic/shared/jumandic_spec.h"
#include "util/logging.hpp"
#include "util/mmap.h"

using namespace jumanpp;

struct BootstrapArgs {
  std::string rawDicPath;
  std::string rawDicVersion;
  std::string outputPath;
};

Status importDictionary(const BootstrapArgs& bargs) {
  core::spec::AnalysisSpec spec;
  JPP_RETURN_IF_ERROR(jumandic::SpecFactory::makeSpec(&spec));
  std::cout << "spec is valid!\n";

  util::FullyMappedFile file;
  JPP_RETURN_IF_ERROR(file.open(bargs.rawDicPath, util::MMapType::ReadOnly));

  std::string name;
  bool progressOk = true;
  auto progress = core::progressCallback(
      [&](u64 current, u64 total) {
        if (current == total) {
          if (!progressOk) {
            std::cout << "\r" << name << " done!" << std::endl;
            progressOk = true;
          }
        } else {
          float percent = static_cast<float>(current) / total * 100;
          std::cout << "\r" << name << ".." << percent << "%          "
                    << std::flush;
        }
      },
      [&](StringPiece newName) { newName.assignTo(name); });

  core::dic::DictionaryBuilder builder;
  JPP_RETURN_IF_ERROR(builder.importSpec(&spec));
  builder.setProgress(&progress);
  JPP_RETURN_IF_ERROR(builder.importCsv(bargs.rawDicPath, file.contents()));
  std::cout << "\nimport done\n";

  core::dic::DictionaryHolder holder;
  JPP_RETURN_IF_ERROR(holder.load(builder.result()));

  core::model::ModelInfo minfo{};
  minfo.parts.emplace_back();
  JPP_RETURN_IF_ERROR(
      builder.fillModelPart(&minfo.parts.back(), bargs.rawDicVersion));

  std::cout << "saving dictionary...";
  core::model::ModelSaver saver;
  JPP_RETURN_IF_ERROR(saver.open(bargs.outputPath));
  JPP_RETURN_IF_ERROR(saver.save(minfo))
  std::cout << "done!\n";

  return Status::Ok();
}

Status parseArgs(BootstrapArgs* bargs, int argc, const char* argv[]) {
  args::ArgumentParser parser{"Juman++ Dictionary bootstrap binary"};
  args::Positional<std::string> input{parser, "INPUT",
                                      "Path of dictionary in CSV format"};
  args::Positional<std::string> output{parser, "OUTPUT",
                                       "Built dictionary will be saved here"};
  args::ValueFlag<std::string> dicVersion{
      parser,
      "VERSION",
      "Embed this version into built dictionary",
      {"dic-version"},
      ""};
  args::HelpFlag help{parser, "HELP", "Print help", {"help", 'h'}};

  try {
    parser.ParseCLI(argc, argv);
  } catch (args::Help&) {
    std::cout << parser;
    exit(1);
  } catch (args::ParseError& e) {
    Status::InvalidParameter() << e.what() << "\n" << parser;
  } catch (...) {
    return Status::InvalidParameter();
  }

  bargs->outputPath = output.Get();
  bargs->rawDicPath = input.Get();
  bargs->rawDicVersion = dicVersion.Get();

  return Status::Ok();
}

int main(int argc, const char* argv[]) {
  BootstrapArgs bargs;
  Status s = parseArgs(&bargs, argc, argv);
  if (!s) {
    LOG_ERROR() << "Failed to parse command line: " << s;
    return 1;
  }

  s = importDictionary(bargs);
  if (!s.isOk()) {
    LOG_ERROR() << s;
    return 1;
  }
  return 0;
}