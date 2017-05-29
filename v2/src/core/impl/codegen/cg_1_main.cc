//
// Created by Arseny Tolmachev on 2017/05/26.
//

#include <iostream>
#include "cg_1_spec.h"
#include "core/core.h"
#include "core/dic_builder.h"
#include "core/dictionary.h"
#include "core/impl/feature_codegen.h"
#include "core/spec/spec_dsl.h"

namespace cg = jumanpp::core::features::codegen;

int main(int argc, char** argv) {
  if (argc != 4) {
    return 1;
  }

  cg::FeatureCodegenConfig conf;
  conf.filename = argv[1];
  conf.className = argv[2];
  conf.baseDirectory = argv[3];

  jumanpp::core::spec::dsl::ModelSpecBuilder bldr;
  jumanpp::codegentest::CgOneSpecFactory::fillSpec(bldr);

  jumanpp::core::spec::AnalysisSpec spec;
  auto s = bldr.build(&spec);
  if (!s) {
    std::cerr << "failed to build spec: " << s.message;
    return 1;
  }

  jumanpp::core::dic::DictionaryBuilder dbld;
  s = dbld.importSpec(&spec);
  s = dbld.importCsv("none", "");
  if (!s) {
    std::cerr << "failed to import empty dic: " << s.message;
    return 1;
  }

  jumanpp::core::dic::DictionaryHolder dh;
  if (!(s = dh.load(dbld.result()))) {
    std::cerr << "failed to build dicholder " << s.message;
    return 1;
  }

  jumanpp::core::RuntimeInfo runtimeInfo;
  if (!dh.compileRuntimeInfo(spec, &runtimeInfo)) {
    return 1;
  }

  jumanpp::core::CoreHolder ch{runtimeInfo, dh};

  jumanpp::core::features::FeatureHolder fh;

  if (!jumanpp::core::features::makeFeatures(ch, nullptr, &fh)) {
    return 1;
  }

  cg::StaticFeatureCodegen sfc{conf};

  s = sfc.generateAndWrite(fh);
  if (!s) {
    std::cerr << "failed to create static files for " << conf.filename << ": "
              << s.message;
    return 1;
  }

  return 0;
}