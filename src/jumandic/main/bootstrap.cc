//
// Created by Arseny Tolmachev on 2017/03/08.
//

#include <iostream>
#include "core/dic/dic_builder.h"
#include "core/dic/dictionary.h"
#include "core/dic/progress.h"
#include "core/impl/model_io.h"
#include "jumandic/shared/jumandic_spec.h"
#include "util/logging.hpp"
#include "util/mmap.h"

using namespace jumanpp;

Status importDictionary(StringPiece path, StringPiece target) {
  core::spec::AnalysisSpec spec;
  JPP_RETURN_IF_ERROR(jumandic::SpecFactory::makeSpec(&spec));
  std::cout << "spec is valid!\n";

  util::MappedFile file;
  JPP_RETURN_IF_ERROR(file.open(path, util::MMapType::ReadOnly));
  util::MappedFileFragment frag;
  JPP_RETURN_IF_ERROR(file.map(&frag, 0, file.size()));

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
  JPP_RETURN_IF_ERROR(builder.importCsv(path, frag.asStringPiece()));
  std::cout << "\nimport done\n";

  core::dic::DictionaryHolder holder;
  JPP_RETURN_IF_ERROR(holder.load(builder.result()));

  core::model::ModelInfo minfo{};
  minfo.parts.emplace_back();
  JPP_RETURN_IF_ERROR(builder.fillModelPart(&minfo.parts.back()));

  std::cout << "saving dictionary...";
  core::model::ModelSaver saver;
  JPP_RETURN_IF_ERROR(saver.open(target));
  JPP_RETURN_IF_ERROR(saver.save(minfo))
  std::cout << "done!\n";

  return Status::Ok();
}

int main(int argc, char* argv[]) {
  if (argc != 3) {
    LOG_ERROR() << "invalid args, must provide dictionary as second, and "
                   "target as third parameters";
  }

  auto filename = StringPiece::fromCString(argv[1]);
  auto target = StringPiece::fromCString(argv[2]);

  Status s = importDictionary(filename, target);
  if (!s.isOk()) {
    LOG_ERROR() << s;
    return 1;
  }
  return 0;
}