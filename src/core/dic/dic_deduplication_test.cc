//
// Created by Arseny Tolmachev on 2017/11/08.
//

#include "core/test/test_analyzer_env.h"

using namespace jumanpp;
using namespace jumanpp::core;

TEST_CASE("we create aliasing dic entries for duplicated dic lines") {
  StringPiece dic{"x,x,x,x\ny,y,y,y\nz,z,z,z\nz,z,a,a\n"};
  tests::PrimFeatureTestEnv env{
      dic, [](spec::dsl::ModelSpecBuilder& bld, tests::FeatureSet fs) {
        bld.field(4, "dummy").strings();
      }};
  auto& spec = env.spec();
  auto& aset = spec.dictionary.aliasingSet;
  CHECK(aset.size() == 2);
  CHECK(aset[0] == 0);
  CHECK(aset[1] == 1);
  env.analyze2("zy");
  auto n = env.uniqueNode("z", 0);
  CHECK(n.eptr.isAlias());
  auto& om = env.output();
  analysis::StringField fd;
  REQUIRE(om.stringField("dummy", &fd));
  auto nw = om.nodeWalker();
  REQUIRE(om.locate(n.eptr, &nw));
  CHECK(nw.remaining() == 2);
  REQUIRE(nw.next());
  CHECK(nw.remaining() == 1);
  CHECK(fd[nw] == "z");
  REQUIRE(nw.next());
  CHECK(nw.remaining() == 0);
  CHECK(fd[nw] == "a");
  CHECK_FALSE(nw.next());
  auto n2 = env.uniqueNode("y", 1);
  CHECK_FALSE(n2.eptr.isAlias());
}