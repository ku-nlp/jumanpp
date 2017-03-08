//
// Created by Arseny Tolmachev on 2017/03/08.
//

#include "core/test/test_analyzer_env.h"

using namespace tests;

TEST_CASE("at least some scores can be computed") {
  StringPiece dic = "XXX,z,KANA\na,b,\nb,c,\n";
  PrimFeatureTestEnv env{
      dic, [](dsl::ModelSpecBuilder& specBldr, FeatureSet& fs) {}};
  env.analyze2("ab");
}

TEST_CASE("at least some scores can be computed with multiple paths") {
  StringPiece dic = "XXX,z,KANA\na,b,\nb,c,\naf,b,\nfb,c,\nf,a,\n";
  PrimFeatureTestEnv env{
      dic, [](dsl::ModelSpecBuilder& specBldr, FeatureSet& fs) {}, 2};
  env.analyze2("afb");
}