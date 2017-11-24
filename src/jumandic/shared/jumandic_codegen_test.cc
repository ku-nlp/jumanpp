//
// Created by Arseny Tolmachev on 2017/11/23.
//

#include <testing/test_analyzer.h>
#include "core/impl/feature_impl_prim.h"
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
      0.1f,  0.2f,   0.3f,   0.4f,  -0.1f, -0.2f,  -0.3f,  -0.4f,
      0.03f, -0.03f, -0.05f, 0.05f, 0.01f, -0.01f, 0.07f,  -0.07f,
      -0.1f, -0.2f,  -0.3f,  -0.4f, 0.03f, -0.03f, -0.05f, 0.05f,
      0.1f,  0.2f,   0.3f,   0.4f,  0.1f,  0.2f,   0.3f,   0.4f,
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
      auto b1 = s1->beamData().row(entry);
      auto b2 = s2->beamData().row(entry);
      for (int bidx = 0; bidx < tenv.beamSize; ++bidx) {
        CAPTURE(bidx);
        auto& be1 = b1.at(bidx);
        auto& be2 = b2.at(bidx);
        if (analysis::EntryBeam::isFake(be1) &&
            analysis::EntryBeam::isFake(be2)) {
          continue;
        }
        CHECK(be1.totalScore == Approx(be2.totalScore));
        CHECK(be1.ptr == be2.ptr);
        auto& pr1 = *be1.ptr.previous;
        auto& pr2 = *be2.ptr.previous;
        CHECK(pr1 == pr2);
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

TEST_CASE(
    "feature representation of gen/nongen is the same with full lattice (2)") {
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
      0.1f,  0.2f,   0.3f,   0.4f,  -0.1f, -0.2f,  -0.3f,  -0.4f,
      0.03f, -0.03f, -0.05f, 0.05f, 0.01f, -0.01f, 0.07f,  -0.07f,
      -0.1f, -0.2f,  -0.3f,  -0.4f, 0.03f, -0.03f, -0.05f, 0.05f,
      0.1f,  0.2f,   0.3f,   0.4f,  0.1f,  0.2f,   0.3f,   0.4f,
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

  auto l2 = nogen.lattice();
  auto l1 = gen.lattice();
  auto nbnd = l2->createdBoundaryCount();

  for (int bndIdx = 2; bndIdx < nbnd; ++bndIdx) {
    CAPTURE(bndIdx);

    auto bnd1 = l1->boundary(bndIdx);
    auto bnd2 = l2->boundary(bndIdx);

    auto& proc1 = gen.sproc();
    auto& proc2 = nogen.sproc();
    features::impl::PrimitiveFeatureContext pfc{
        gen.extraNodesContext(), tenv.dic.fields(), tenv.dic.entries(),
        nogen.input().codepoints()};

    auto nels = bnd1->localNodeCount();
    proc1.startBoundary(nels);
    proc2.startBoundary(nels);
    proc1.computeT0All(bndIdx, &hfp, &pfc);
    proc2.applyT0(bndIdx, &hfp);

    auto t0s1 = proc1.scores_.bufferT0();
    auto t0s2 = proc2.scores_.bufferT0();
    auto& fb1 = proc1.featureBuffer_;
    auto& fb2 = proc2.featureBuffer_;

    for (int el = 0; el < nels; ++el) {
      CAPTURE(el);
      CHECK(t0s1.at(el) == Approx(t0s2.at(el)));
    }
    auto t1x1 = fb1.t1Buf(gen.ngramStats().num2Grams, nels);
    auto t1x2 = fb2.t1Buf(gen.ngramStats().num2Grams, nels);
    for (int row = 0; row < t1x1.numRows(); ++row) {
      for (int col = 0; col < t1x1.rowSize(); ++col) {
        CHECK(t1x1.row(row).at(col) == t1x2.row(row).at(col));
      }
    }
    auto t2x1 = fb1.t2Buf1(gen.ngramStats().num3Grams, nels);
    auto t2x2 = fb2.t2Buf1(gen.ngramStats().num3Grams, nels);
    for (int row = 0; row < t2x1.numRows(); ++row) {
      for (int col = 0; col < t2x1.rowSize(); ++col) {
        CHECK(t2x1.row(row).at(col) == t2x2.row(row).at(col));
      }
    }
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