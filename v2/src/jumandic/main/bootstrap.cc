//
// Created by Arseny Tolmachev on 2017/03/08.
//

#include "jumandic/shared/jumandic_spec.h"
#include "core/dic_builder.h"
#include "util/mmap.h"
#include "util/logging.hpp"
#include "core/impl/model_io.h"
#include "core/dictionary.h"

using namespace jumanpp;

Status importDictionary(StringPiece path, StringPiece target) {
  core::spec::AnalysisSpec spec;
  jumandic::SpecFactory::makeSpec(&spec);

  util::MappedFile file;
  JPP_RETURN_IF_ERROR(file.open(path, util::MMapType::ReadOnly));
  util::MappedFileFragment frag;
  JPP_RETURN_IF_ERROR(file.map(&frag, 0, file.size()));

  core::dic::DictionaryBuilder builder;
  JPP_RETURN_IF_ERROR(builder.importSpec(&spec));
  JPP_RETURN_IF_ERROR(builder.importCsv(path, frag.asStringPiece()));


  core::dic::DictionaryHolder holder;
  JPP_RETURN_IF_ERROR(holder.load(builder.result()));

  core::RuntimeInfo runtime;
  JPP_RETURN_IF_ERROR(holder.compileRuntimeInfo(builder.spec(), &runtime));

  core::model::ModelInfo minfo {};
  minfo.parts.emplace_back();
  JPP_RETURN_IF_ERROR(builder.fillModelPart(runtime, &minfo.parts.back()));


  core::model::ModelSaver saver;
  JPP_RETURN_IF_ERROR(saver.open(target));
  JPP_RETURN_IF_ERROR(saver.save(minfo));

  return Status::Ok();
}

int main(int argc, char* argv[]) {

  if (argc != 3) {
    LOG_ERROR() << "invalid args, must provide dictionary as second, and target as third parameters";
  }

  StringPiece filename{argv[1]};
  StringPiece target{argv[2]};

  Status s = importDictionary(filename, target);
  if (!s.isOk()) {
    LOG_ERROR() << s.message;
    return 1;
  }
  return 0;
}