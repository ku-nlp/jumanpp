//
// Created by Arseny Tolmachev on 2017/11/23.
//

#include "jpp_jumandic_cg.h"
#include "jumandic_id_resolver.h"
#include "jumandic_spec.h"
#include "testing/test_analyzer.h"
#include "util/mmap.h"

using namespace jumanpp::core;
using namespace jumanpp;

TEST_CASE(
    "feature representation of gen/nongen is the same with full lattice") {
  jumanpp::testing::TestEnv tenv;
  tenv.beamSize = 5;
  tenv.spec([](spec::dsl::ModelSpecBuilder& bldr) {
    jumanpp::jumandic::SpecFactory::fillSpec(bldr);
  });
  util::MappedFile fl;
  REQUIRE_OK(fl.open("jumandic/codegen.mdic", util::MMapType::ReadOnly));
  util::MappedFileFragment frag;
  REQUIRE_OK(fl.map(&frag, 0, fl.size()));
  tenv.importDic(frag.asStringPiece(), "codegen.mdic");
  float weights[] = {
      0.1f, 0.2f, 0.3f, 0.4f, -0.1f, -0.2f, -0.3f, -0.4f,
  };
  analysis::HashedFeaturePerceptron hfp{weights};
  analysis::ScorerDef sdef;
  sdef.feature = &hfp;
  sdef.scoreWeights.push_back(1.0f);
  ScoringConfig sconf{5, 1};
  testing::TestAnalyzer nogen{tenv.core.get(), sconf, tenv.aconf};
  REQUIRE(nogen.initScorers(sdef));

  CoreHolder core2{tenv.core->spec(), tenv.core->dic()};
  jumanpp_generated::JumandicStatic js;
  REQUIRE(core2.initialize(&js));
  testing::TestAnalyzer gen{&core2, sconf, tenv.aconf};
  REQUIRE(gen.initScorers(sdef));

  nogen.fullAnalyze("５５１年もガラフケマペが兵をつの〜ってたな！", &sdef);
  gen.fullAnalyze("５５１年もガラフケマペが兵をつの〜ってたな！", &sdef);

  auto nbnd = nogen.lattice()->createdBoundaryCount();

  for (int bndIdx = 2; bndIdx < nbnd; ++bndIdx) {
    CAPTURE(bndIdx);
    auto bnd1 = gen.lattice()->boundary(bndIdx);
    auto bnd2 = nogen.lattice()->boundary(bndIdx);

    auto s1 = bnd1->starts();
    auto s2 = bnd2->starts();
    REQUIRE(s1->numEntries() == s2->numEntries());
    for (int entry = 0; entry < s1->numEntries(); ++entry) {
      auto r1 = s1->patternFeatureData().row(entry);
      auto r2 = s2->patternFeatureData().row(entry);
      REQUIRE(r1.size() <= r2.size());
      CAPTURE(entry);
      for (int feature = 0; feature < r1.size(); ++feature) {
        CAPTURE(feature);
        CHECK(r1.at(feature) == r2.at(feature));
      }
    }
  }

  auto lbeam1 =
      nogen.lattice()->boundary(nbnd - 1)->starts()->beamData().row(0);
  auto lbeam2 = gen.lattice()->boundary(nbnd - 1)->starts()->beamData().row(0);

  for (int i = 0; i < tenv.beamSize; ++i) {
    CAPTURE(i);
    CHECK(lbeam1.at(i).totalScore == Approx(lbeam2.at(i).totalScore));
  }
}

TEST_CASE("feature representation of gen/nongen is the same with global beam") {
  jumanpp::testing::TestEnv tenv;
  tenv.beamSize = 5;
  tenv.aconf.globalBeamSize = 5;
  tenv.aconf.rightGbeamCheck = 1;
  tenv.aconf.rightGbeamSize = 5;
  tenv.spec([](spec::dsl::ModelSpecBuilder& bldr) {
    jumanpp::jumandic::SpecFactory::fillSpec(bldr);
  });
  util::MappedFile fl;
  REQUIRE_OK(fl.open("jumandic/codegen.mdic", util::MMapType::ReadOnly));
  util::MappedFileFragment frag;
  REQUIRE_OK(fl.map(&frag, 0, fl.size()));
  tenv.importDic(frag.asStringPiece(), "codegen.mdic");
  float weights[] = {
      0.1f, 0.2f, 0.3f, 0.4f, -0.1f, -0.2f, -0.3f, -0.4f,
  };
  analysis::HashedFeaturePerceptron hfp{weights};
  analysis::ScorerDef sdef;
  sdef.feature = &hfp;
  sdef.scoreWeights.push_back(1.0f);
  ScoringConfig sconf{5, 1};
  testing::TestAnalyzer nogen{tenv.core.get(), sconf, tenv.aconf};
  REQUIRE(nogen.initScorers(sdef));

  CoreHolder core2{tenv.core->spec(), tenv.core->dic()};
  jumanpp_generated::JumandicStatic js;
  REQUIRE(core2.initialize(&js));
  testing::TestAnalyzer gen{&core2, sconf, tenv.aconf};
  REQUIRE(gen.initScorers(sdef));

  nogen.fullAnalyze("５５１年もガラフケマペが兵をつの〜ってたな！", &sdef);
  gen.fullAnalyze("５５１年もガラフケマペが兵をつの〜ってたな！", &sdef);

  auto nbnd = nogen.lattice()->createdBoundaryCount();

  for (int bndIdx = 2; bndIdx < nbnd; ++bndIdx) {
    CAPTURE(bndIdx);
    auto bnd1 = gen.lattice()->boundary(bndIdx);
    auto bnd2 = nogen.lattice()->boundary(bndIdx);

    auto s1 = bnd1->starts();
    auto s2 = bnd2->starts();
    REQUIRE(s1->numEntries() == s2->numEntries());
    for (int entry = 0; entry < s1->numEntries(); ++entry) {
      auto r1 = s1->patternFeatureData().row(entry);
      auto r2 = s2->patternFeatureData().row(entry);
      REQUIRE(r1.size() <= r2.size());
      CAPTURE(entry);
      for (int feature = 0; feature < r1.size(); ++feature) {
        CAPTURE(feature);
        CHECK(r1.at(feature) == r2.at(feature));
      }
    }
  }

  auto lbeam1 =
      nogen.lattice()->boundary(nbnd - 1)->starts()->beamData().row(0);
  auto lbeam2 = gen.lattice()->boundary(nbnd - 1)->starts()->beamData().row(0);

  for (int i = 0; i < tenv.beamSize; ++i) {
    CAPTURE(i);
    CHECK(lbeam1.at(i).totalScore == Approx(lbeam2.at(i).totalScore));
  }
}