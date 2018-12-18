//
// Created by Arseny Tolmachev on 2017/11/23.
//

#include <testing/test_analyzer.h>
#include <fstream>
#include "core/impl/feature_impl_prim.h"
#include "core/impl/graphviz_format.h"
#include "jpp_jumandic_cg.h"
#include "jumandic_id_resolver.h"
#include "jumandic_spec.h"
#include "testing/test_analyzer.h"
#include "util/logging.hpp"
#include "util/mmap.h"

using namespace jumanpp::core;
using namespace jumanpp;

namespace {

void dumpJumandicLattice(std::string dst,
                         const core::analysis::AnalyzerImpl& ai) {
  core::format::GraphVizBuilder gb;
  gb.row({"canonic"});
  gb.row({"surface"});
  gb.row({"pos", "subpos"});
  gb.row({"conjform", "conjtype"});
  core::format::GraphVizFormat fmt;
  REQUIRE(gb.build(&fmt, 5));
  REQUIRE(fmt.initialize(ai.output()));
  REQUIRE(fmt.render(ai.lattice()));
  std::ofstream of(dst);
  of << fmt.result();
}

}  // namespace

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
  alignas(64) float weights[] = {
      // clang-format off
    -1.2461f, -1.3578f, -0.9778f, -1.2633f, -0.5212f, -1.4829f, -0.1640f, +1.3277f,
    +1.1113f, -0.4886f, +1.4962f, -0.3044f, +1.4552f, +0.3961f, +0.5759f, -0.6813f,
    -0.5353f, +1.2839f, +1.1457f, -0.6340f, +0.4979f, -0.6948f, -0.3114f, +0.5241f,
    +0.5556f, -0.2019f, -0.8539f, +0.3290f, +1.6552f, +0.1596f, -0.1151f, +0.3486f,
    +0.2031f, +0.4672f, -0.4492f, -0.3219f, -1.2481f, +2.4275f, +0.6157f, -0.0480f,
    +0.3908f, -0.7122f, +1.5255f, +0.4294f, -0.8130f, +1.2768f, -0.2822f, +1.5535f,
    -0.7774f, -0.2359f, +2.1092f, +0.8612f, +0.3645f, -1.7273f, +1.4427f, +1.8558f,
    +0.0138f, +0.9191f, +0.6721f, +0.7195f, -0.1228f, -0.1717f, -1.7961f, +1.0603f,
      // clang-format on
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

  REQUIRE(
      nogen.fullAnalyze("５５１年もガラフケマペが兵をつの〜ってたな！", &sdef));
  REQUIRE(
      gen.fullAnalyze("５５１年もガラフケマペが兵をつの〜ってたな！", &sdef));

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
  alignas(64) float weights[] = {
      // clang-format off
    -1.2461f, -1.3578f, -0.9778f, -1.2633f, -0.5212f, -1.4829f, -0.1640f, +1.3277f,
    +1.1113f, -0.4886f, +1.4962f, -0.3044f, +1.4552f, +0.3961f, +0.5759f, -0.6813f,
    -0.5353f, +1.2839f, +1.1457f, -0.6340f, +0.4979f, -0.6948f, -0.3114f, +0.5241f,
    +0.5556f, -0.2019f, -0.8539f, +0.3290f, +1.6552f, +0.1596f, -0.1151f, +0.3486f,
    +0.2031f, +0.4672f, -0.4492f, -0.3219f, -1.2481f, +2.4275f, +0.6157f, -0.0480f,
    +0.3908f, -0.7122f, +1.5255f, +0.4294f, -0.8130f, +1.2768f, -0.2822f, +1.5535f,
    -0.7774f, -0.2359f, +2.1092f, +0.8612f, +0.3645f, -1.7273f, +1.4427f, +1.8558f,
    +0.0138f, +0.9191f, +0.6721f, +0.7195f, -0.1228f, -0.1717f, -1.7961f, +1.0603f,
      // clang-format on
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

  REQUIRE(
      nogen.fullAnalyze("５５１年もガラフケマペが兵をつの〜ってたな！", &sdef));
  REQUIRE(
      gen.fullAnalyze("５５１年もガラフケマペが兵をつの〜ってたな！", &sdef));

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
    auto t0s1 = proc1.scores_.bufferT0();
    auto t0s2 = proc2.scores_.bufferT0();
    util::fill(t0s1, 1000);
    util::fill(t0s2, 1000);
    proc1.computeT0All(bndIdx, &hfp, &pfc);
    proc2.applyT0(bndIdx, &hfp);

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

    auto e1 = bnd1->ends();
    for (int t1idx = 0; t1idx < e1->nodePtrs().size(); ++t1idx) {
      CAPTURE(t1idx);
      auto& t1node = e1->nodePtrs().at(t1idx);
      proc1.applyT1(t1node.boundary, t1node.position, &hfp);
      proc2.applyT1(t1node.boundary, t1node.position, &hfp);

      auto t2y1 = fb1.t2Buf2(gen.ngramStats().num3Grams, nels);
      auto t2y2 = fb2.t2Buf2(gen.ngramStats().num3Grams, nels);
      for (int row = 0; row < t2y1.numRows(); ++row) {
        for (int col = 0; col < t2y1.rowSize(); ++col) {
          CHECK(t2y1.row(row).at(col) == t2y2.row(row).at(col));
        }
      }

      auto t1s1 = proc1.scores_.bufferT1();
      auto t1s2 = proc2.scores_.bufferT1();
      for (int el = 0; el < nels; ++el) {
        CAPTURE(el);
        CHECK(t1s1.at(el) == Approx(t1s2.at(el)));
      }
      proc1.resolveBeamAt(t1node.boundary, t1node.position);
      proc2.resolveBeamAt(t1node.boundary, t1node.position);

      i32 abeam1 = proc1.activeBeamSize();
      i32 abeam2 = proc2.activeBeamSize();
      CHECK(abeam1 == abeam2);
      for (int beamIdx = 0; beamIdx < abeam1; ++beamIdx) {
        proc1.applyT2(beamIdx, &hfp);
        proc2.applyT2(beamIdx, &hfp);
        auto t2s1 = proc1.scores_.bufferT2();
        auto t2s2 = proc1.scores_.bufferT2();
        for (int el = 0; el < nels; ++el) {
          CAPTURE(el);
          CHECK(t2s1.at(el) == Approx(t2s2.at(el)));
        }
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
  alignas(64) float weights[] = {
      // clang-format off
    -1.2461f, -1.3578f, -0.9778f, -1.2633f, -0.5212f, -1.4829f, -0.1640f, +1.3277f,
    +1.1113f, -0.4886f, +1.4962f, -0.3044f, +1.4552f, +0.3961f, +0.5759f, -0.6813f,
    -0.5353f, +1.2839f, +1.1457f, -0.6340f, +0.4979f, -0.6948f, -0.3114f, +0.5241f,
    +0.5556f, -0.2019f, -0.8539f, +0.3290f, +1.6552f, +0.1596f, -0.1151f, +0.3486f,
    +0.2031f, +0.4672f, -0.4492f, -0.3219f, -1.2481f, +2.4275f, +0.6157f, -0.0480f,
    +0.3908f, -0.7122f, +1.5255f, +0.4294f, -0.8130f, +1.2768f, -0.2822f, +1.5535f,
    -0.7774f, -0.2359f, +2.1092f, +0.8612f, +0.3645f, -1.7273f, +1.4427f, +1.8558f,
    +0.0138f, +0.9191f, +0.6721f, +0.7195f, -0.1228f, -0.1717f, -1.7961f, +1.0603f,
      // clang-format on
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

  REQUIRE(
      nogen.fullAnalyze("５５１年もガラフケマペが兵をつの〜ってたな！", &sdef));
  REQUIRE(
      gen.fullAnalyze("５５１年もガラフケマペが兵をつの〜ってたな！", &sdef));

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
      for (u32 beam = 0; beam < b1.size(); ++beam) {
        auto& be1 = b1.at(beam);
        auto& be2 = b2.at(beam);

        bool fake1 = core::analysis::EntryBeam::isFake(be1);
        bool fake2 = core::analysis::EntryBeam::isFake(be2);
        if (fake1 && fake2) {
          continue;
        }

        if ((!fake1 && fake2) || (fake1 && !fake2)) {
          INFO(fake1);
          INFO(fake2);
          FAIL("one of beams are bad");
        }

        CAPTURE(beam);
        CHECK(be1.totalScore == Approx(be2.totalScore));
        CHECK(be1.ptr == be2.ptr);
      }
    }
  }

  auto lbeam1 =
      nogen.lattice()->boundary(nbnd - 1)->starts()->beamData().row(0);
  auto lbeam2 = gen.lattice()->boundary(nbnd - 1)->starts()->beamData().row(0);

  for (int i = 0; i < tenv.beamSize; ++i) {
    CAPTURE(i);
    CHECK(lbeam1.at(i).totalScore == Approx(lbeam2.at(i).totalScore));
    // LOG_WARN() << lbeam1.at(i).totalScore;
    // LOG_WARN() << lbeam2.at(i).totalScore;
  }

  // dumpJumandicLattice("/tmp/jpp/gen.dot", gen);
  // dumpJumandicLattice("/tmp/jpp/nogen.dot", nogen);
}