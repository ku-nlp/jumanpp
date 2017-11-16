//
// Created by Arseny Tolmachev on 2017/11/16.
//

#include "testing/test_analyzer.h"

using namespace jumanpp;
using namespace jumanpp::testing;

TEST_CASE("analyzer with three differently aligned fields work") {
  TestEnv env;
  env.spec([](core::spec::dsl::ModelSpecBuilder& bldr) {
    auto& a = bldr.field(1, "a").strings().trieIndex().align(2);
    auto& b = bldr.field(2, "b").strings().align(3);
    bldr.field(3, "c").strings().align(4);
    bldr.unigram({a, b});
  });
  env.importDic("a,b,c\nc,a,d\nf,a,g\np,a,r\n");
  auto& spec = env.restoredDic.spec;
  auto& flds = spec.dictionary.fields;
  CHECK(flds[0].alignment == 2);
  CHECK(flds[1].alignment == 3);
  CHECK(flds[2].alignment == 4);
  AnalyzerMethods am;
  REQUIRE(am.initialize(env.analyzer.get()));
  env.analyzer->resetForInput("acfp");
  REQUIRE(env.analyzer->prepareNodeSeeds());
  REQUIRE(env.analyzer->buildLattice());
  auto n1 = am.firstNode({2, 0});
  CHECK(n1 == ExampleData{"a", "b", "c"});
  auto n2 = am.firstNode({3, 0});
  CHECK(n2 == ExampleData{"c", "a", "d"});
  auto n3 = am.firstNode({4, 0});
  CHECK(n3 == ExampleData{"f", "a", "g"});
  auto n4 = am.firstNode({5, 0});
  CHECK(n4 == ExampleData{"p", "a", "r"});
}

TEST_CASE("analyzer with shared and aligned fields work") {
  TestEnv env;
  env.spec([](core::spec::dsl::ModelSpecBuilder& bldr) {
    auto& a = bldr.field(1, "a").strings().trieIndex().align(2);
    auto& b = bldr.field(2, "b").strings().stringStorage(a);
    bldr.field(3, "c").strings().stringStorage(a);
    bldr.unigram({a, b});
  });
  env.importDic("a,b,c\nc,a,d\nf,a,g\np,a,r\n");
  auto& spec = env.restoredDic.spec;
  auto& flds = spec.dictionary.fields;
  CHECK(flds[0].alignment == 2);
  CHECK(flds[1].alignment == 2);
  CHECK(flds[2].alignment == 2);
  AnalyzerMethods am;
  REQUIRE(am.initialize(env.analyzer.get()));
  env.analyzer->resetForInput("acfp");
  REQUIRE(env.analyzer->prepareNodeSeeds());
  REQUIRE(env.analyzer->buildLattice());
  auto n1 = am.firstNode({2, 0});
  CHECK(n1 == ExampleData{"a", "b", "c"});
  auto n2 = am.firstNode({3, 0});
  CHECK(n2 == ExampleData{"c", "a", "d"});
  auto n3 = am.firstNode({4, 0});
  CHECK(n3 == ExampleData{"f", "a", "g"});
  auto n4 = am.firstNode({5, 0});
  CHECK(n4 == ExampleData{"p", "a", "r"});
}