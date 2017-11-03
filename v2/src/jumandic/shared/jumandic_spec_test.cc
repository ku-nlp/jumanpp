//
// Created by Arseny Tolmachev on 2017/03/05.
//

#include "jumandic_spec.h"
#include "testing/standalone_test.h"

using namespace jumanpp;

TEST_CASE("jumandic spec is valid") {
  core::spec::AnalysisSpec spec;
  CHECK_OK(jumandic::SpecFactory::makeSpec(&spec));
  CHECK(spec.dictionary.fields.size() == 9);
  CHECK(spec.dictionary.indexColumn == 0);
  CHECK(spec.features.ngram.size() == 67);
  CHECK(spec.unkCreators.size() == 7);
}