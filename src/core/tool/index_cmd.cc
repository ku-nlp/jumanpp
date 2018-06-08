//
// Created by Arseny Tolmachev on 2018/06/08.
//

#include "index_cmd.h"
#include "core/dic/dic_builder.h"
#include "core/impl/model_io.h"
#include "core/spec/spec_parser.h"
#include "util/mmap.h"

namespace jumanpp {
namespace core {
namespace tool {

struct IndexToolImpl {
  util::FullyMappedFile dicFile;
  dic::DictionaryBuilder builder;
  spec::AnalysisSpec rawSpec;
};

IndexTool::IndexTool() { impl_.reset(new IndexToolImpl); }

IndexTool::~IndexTool() {}

void IndexTool::setProgressCallback(ProgressCallback* callback) {
  impl_->builder.setProgress(callback);
}

Status IndexTool::indexDictionary(StringPiece specFile, StringPiece dicFile) {
  JPP_RETURN_IF_ERROR(spec::parseFromFile(specFile, &impl_->rawSpec));
  JPP_RETURN_IF_ERROR(impl_->builder.importSpec(&impl_->rawSpec));

  JPP_RETURN_IF_ERROR(impl_->dicFile.open(dicFile, util::MMapType::ReadOnly));
  JPP_RETURN_IF_ERROR(
      impl_->builder.importCsv(dicFile, impl_->dicFile.contents()));
  return Status::Ok();
}

Status IndexTool::saveModel(StringPiece outputFile, StringPiece dicComment) {
  core::model::ModelSaver saver;
  core::model::ModelInfo info;
  info.parts.emplace_back();
  auto& part = info.parts.back();

  JPP_RETURN_IF_ERROR(impl_->builder.fillModelPart(&part, dicComment));
  JPP_RETURN_IF_ERROR(saver.open(outputFile));
  JPP_RETURN_IF_ERROR(saver.save(info));

  return Status::Ok();
}

}  // namespace tool
}  // namespace core
}  // namespace jumanpp