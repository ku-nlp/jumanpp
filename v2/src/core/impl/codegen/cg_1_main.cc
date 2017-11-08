//
// Created by Arseny Tolmachev on 2017/05/26.
//

#include <iostream>
#include "cg_1_spec.h"
#include "core/core.h"
#include "core/dic/dic_builder.h"
#include "core/dic/dictionary.h"
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
    std::cerr << "failed to build spec: " << s;
    return 1;
  }

  cg::StaticFeatureCodegen sfc{conf, spec};

  s = sfc.generateAndWrite();
  if (!s) {
    std::cerr << "failed to create static files for " << conf.filename << ": "
              << s;
    return 1;
  }

  return 0;
}