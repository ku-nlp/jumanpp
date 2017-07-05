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
  auto& top = env.top();
  ConnectionPtr el;
  CHECK(top.nextBoundary());
  CHECK(top.nextNode(&el));
  auto n1 = env.target(el);
  CHECK(n1.f1 == "a");
  CHECK(n1.f2 == "b");
  CHECK_FALSE(top.nextNode(&el));
  CHECK(top.nextBoundary());
  CHECK(top.nextNode(&el));
  auto n2 = env.target(el);
  CHECK(n2.f1 == "b");
  CHECK(n2.f2 == "c");
  CHECK_FALSE(top.nextNode(&el));
  CHECK_FALSE(top.nextBoundary());
}

TEST_CASE("at least some scores can be computed with multiple paths") {
  StringPiece dic = "XXX,z,KANA\na,b,\nb,c,\naf,b,\nfb,c,\nf,a,\n";
  PrimFeatureTestEnv env{
      dic, [](dsl::ModelSpecBuilder& specBldr, FeatureSet& fs) {}, 2};
  env.analyze2("afb");
  auto& top = env.top();
  ConnectionPtr el;
  CHECK(top.nextBoundary());
  CHECK(top.nextNode(&el));
  auto n1 = env.target(el);
  CHECK(n1.f1 == "a");
  CHECK(n1.f2 == "b");
  CHECK_FALSE(top.nextNode(&el));
  CHECK(top.nextBoundary());
  CHECK(top.nextNode(&el));
  auto n2 = env.target(el);
  CHECK(n2.f1 == "f");
  CHECK(n2.f2 == "a");
  CHECK_FALSE(top.nextNode(&el));
  CHECK(top.nextBoundary());
  CHECK(top.nextNode(&el));
  auto n3 = env.target(el);
  CHECK(n3.f1 == "b");
  CHECK(n3.f2 == "c");
  CHECK_FALSE(top.nextNode(&el));
  CHECK_FALSE(top.nextBoundary());
  //env.dumpTrainers("/tmp/jpp", 5);
}