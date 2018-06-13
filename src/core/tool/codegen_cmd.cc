//
// Created by Arseny Tolmachev on 2018/06/13.
//

#include "codegen_cmd.h"
#include "core/codegen/feature_codegen.h"
#include "core/spec/spec_parser.h"
#include "pathie-cpp/include/path.hpp"

namespace jumanpp {
namespace core {
namespace tool {

Status generateStaticFeatures(StringPiece specFile, StringPiece baseName,
                              StringPiece className) {
  spec::AnalysisSpec spec;
  JPP_RETURN_IF_ERROR(spec::parseFromFile(specFile, &spec));
  Pathie::Path basePath{baseName.str()};
  auto directory = basePath.parent();
  auto baseFile = basePath.basename();

  directory.mktree();

  features::codegen::FeatureCodegenConfig cfg;
  cfg.className = className.str();
  cfg.baseDirectory = directory.str();
  cfg.filename = baseFile.str();

  features::codegen::StaticFeatureCodegen codegen{cfg, spec};
  JPP_RETURN_IF_ERROR(codegen.generateAndWrite());
  return Status::Ok();
}

}  // namespace tool
}  // namespace core
}  // namespace jumanpp