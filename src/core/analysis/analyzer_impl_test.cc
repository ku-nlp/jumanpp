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
  // env.dumpTrainers("/tmp/jpp", 5);
}

TEST_CASE("global beam does not crash") {
  StringPiece dic = "XXX,z,KANA\na,b,\nb,c,\naf,b,\nfb,c,\nf,a,\n";
  PrimFeatureTestEnv env{
      dic, [](dsl::ModelSpecBuilder& specBldr, FeatureSet& fs) {}, 3, 4};
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
  // env.dumpTrainers("/tmp/jpp", 5);
}
namespace {

// Penalizes every node which starts at the given boundary
struct BoundaryPenalty : ScorePlugin {
  u16 boundary;
  explicit BoundaryPenalty(u16 boundary) : boundary{boundary} {}
  bool updateScore(const Lattice* l, const ConnectionPtr& ptr,
                   float* score) const override {
    if (ptr.boundary == boundary) {
      *score -= 100.0f;
    }
    return true;
  }
};

void checkPluginChangesPath(i32 globalBeam) {
  StringPiece dic = "XXX,z,KANA\na,b,\nb,c,\naf,b,\nfb,c,\nf,a,\n";
  PrimFeatureTestEnv env{
      dic, [](dsl::ModelSpecBuilder& specBldr, FeatureSet& fs) {}, 3,
      globalBeam};
  env.analyze2("afb");
  auto& top = env.top();
  ConnectionPtr el;
  REQUIRE(top.nextBoundary());
  REQUIRE(top.nextBoundary());
  REQUIRE(top.nextNode(&el));
  REQUIRE(env.target(el).f1 == "f");

  // "f" and "fb" start at the penalized boundary, only "af" + "b" remains
  BoundaryPenalty plugin{el.boundary};
  env.analyze2("afb", &plugin);
  REQUIRE(top.nextBoundary());
  REQUIRE(top.nextNode(&el));
  CHECK(env.target(el).f1 == "af");
  REQUIRE(top.nextBoundary());
  REQUIRE(top.nextNode(&el));
  CHECK(env.target(el).f1 == "b");
  CHECK_FALSE(top.nextBoundary());
}

}  // namespace

TEST_CASE("plugin affects the path without global beam") {
  checkPluginChangesPath(0);
}

TEST_CASE("plugin affects the path with global beam") {
  checkPluginChangesPath(4);
}
