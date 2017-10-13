//
// Created by Arseny Tolmachev on 2017/06/29.
//

#include "rnn_scorer.h"
#include <fstream>
#include "core/env.h"
#include "core/impl/graphviz_format.h"
#include "rnn/mikolov_rnn.h"
#include "rnn_id_resolver.h"
#include "testing/test_analyzer.h"
#include "util/lazy.h"

using namespace jumanpp;
namespace a = jumanpp::core::analysis;

struct RnnScorerEnv {
  testing::TestEnv env;
  core::JumanppEnv jppEnv;
  std::vector<float> features_{0, 0.f};
  util::Lazy<core::analysis::AnalyzerImpl> anaImpl;
  core::analysis::AnalyzerConfig anaCfg;
  core::ScoringConfig scoreCfg;
  core::analysis::HashedFeaturePerceptron perceptron{features_};

  RnnScorerEnv(StringPiece dic) {
    env.spec([](core::spec::dsl::ModelSpecBuilder& sb) {
      auto a = sb.field(1, "a").strings().trieIndex();
      auto b = sb.field(2, "b").strings();

      sb.unigram({a, b});
    });
    env.saveDic(dic);
    env.loadEnv(&jppEnv);
    jppEnv.setBeamSize(5);
    REQUIRE_OK(jppEnv.initFeatures(nullptr));
    scoreCfg.beamSize = 5;
    scoreCfg.numScorers = 2;
    anaImpl.initialize(jppEnv.coreHolder(), scoreCfg, anaCfg);
  }

  core::analysis::AnalyzerImpl& ana() { return anaImpl.value(); }

  void dumpTrainers(StringPiece baseDir) {
    auto bldr = core::format::GraphVizBuilder{};
    bldr.row({"a", "b"});
    core::format::GraphVizFormat fmt;
    REQUIRE_OK(bldr.build(&fmt, scoreCfg.beamSize));

    char buffer[256];

    std::snprintf(buffer, 255, "%s/dump.dot", baseDir.char_begin());
    std::ofstream out{buffer};
    auto pana = &anaImpl.value();
    REQUIRE_OK(fmt.initialize(pana->output()));
    fmt.reset();
    auto lat = pana->lattice();
    REQUIRE_OK(fmt.render(lat));
    out << fmt.result();
  }
};

void checkItems(a::rnn::RnnIdContainer& c, int boundary, int count,
                int numSpans = 1) {
  CAPTURE(boundary);
  CAPTURE(count);
  CAPTURE(numSpans);
  auto abnd = c.atBoundary(boundary);
  CHECK(abnd.numSpans() == numSpans);
  CHECK(abnd.ids().size() == count);
}

TEST_CASE("RnnIdResolver correctly resolves present ids") {
  RnnScorerEnv env{"a,1\nb,2\nc,3"};
  auto& ana = env.ana();
  ana.resetForInput("bac");
  REQUIRE_OK(ana.prepareNodeSeeds());
  REQUIRE_OK(ana.buildLattice());
  a::rnn::RnnIdResolver idres{};
  jumanpp::rnn::mikolov::MikolovModelReader mrr;
  REQUIRE_OK(mrr.open("rnn/testlm"));
  REQUIRE_OK(mrr.parse());
  REQUIRE_OK(idres.loadFromDic(env.jppEnv.coreHolder()->dic(), "a", mrr.words(),
                               "<unk>"));
  a::rnn::RnnIdContainer ctr{ana.alloc()};
  REQUIRE_OK(idres.resolveIds(&ctr, ana.lattice(), ana.extraNodesContext()));
  checkItems(ctr, 0, 1);
  checkItems(ctr, 1, 1);
  checkItems(ctr, 2, 1);
  checkItems(ctr, 3, 1);
  CHECK(ctr.atBoundary(3).ids().at(0) == 6);
  checkItems(ctr, 4, 1);
  checkItems(ctr, 5, 1);
}

TEST_CASE("RnnIdResolver correctly resolves absent ids") {
  RnnScorerEnv env{"acga,1\nb,2\nc,3"};
  auto& ana = env.ana();
  ana.resetForInput("bacga");
  REQUIRE_OK(ana.prepareNodeSeeds());
  REQUIRE_OK(ana.buildLattice());
  a::rnn::RnnIdResolver idres{};
  jumanpp::rnn::mikolov::MikolovModelReader mrr;
  REQUIRE_OK(mrr.open("rnn/testlm"));
  REQUIRE_OK(mrr.parse());
  REQUIRE_OK(idres.loadFromDic(env.jppEnv.coreHolder()->dic(), "a", mrr.words(),
                               "<unk>"));
  CHECK(idres.unkId() == 2);
  a::rnn::RnnIdContainer ctr{ana.alloc()};
  REQUIRE_OK(idres.resolveIds(&ctr, ana.lattice(), ana.extraNodesContext()));
  checkItems(ctr, 0, 1);
  checkItems(ctr, 1, 1);
  checkItems(ctr, 2, 1);
  checkItems(ctr, 3, 1);
  CHECK(ctr.atBoundary(3).ids().at(0) == -1);
  checkItems(ctr, 4, 1);
  checkItems(ctr, 5, 0);
  checkItems(ctr, 6, 0);
  checkItems(ctr, 7, 1);
}

TEST_CASE("RNN computes scores") {
  RnnScorerEnv env{"a,1\nb,2\nc,3"};
  auto& ana = env.ana();
  jumanpp::rnn::mikolov::MikolovModelReader mrr;
  REQUIRE_OK(mrr.open("rnn/testlm"));
  REQUIRE_OK(mrr.parse());

  a::ScorerDef scorerDef{};
  scorerDef.scoreWeights.push_back(1.0f);
  scorerDef.scoreWeights.push_back(1.0f);
  a::rnn::RnnHolder rnnHolder;
  scorerDef.feature = &env.perceptron;
  REQUIRE_OK(rnnHolder.init({}, mrr, env.jppEnv.coreHolder()->dic(), "a"));
  scorerDef.others.push_back(&rnnHolder);
  REQUIRE_OK(ana.initScorers(scorerDef));
  ana.resetForInput("bac");
  REQUIRE_OK(ana.prepareNodeSeeds());
  REQUIRE_OK(ana.buildLattice());
  REQUIRE_OK(ana.bootstrapAnalysis());
  REQUIRE_OK(ana.computeScores(&scorerDef));
  CHECK(ana.lattice()->createdBoundaryCount() == 6);
  CHECK(!std::isnan(
      ana.lattice()->boundary(5)->starts()->beamData().at(0).totalScore));
}

TEST_CASE("RNN computes scores with unks") {
  RnnScorerEnv env{
      "newsan,12\nn,14\newsan,13\nnew,1\nnews,2\nsan,3\na,4\nan,5\n"
      "apple,6\news,7\nne,20\npple,8\nwsa,9\np,10\nle,11\n"};
  auto& ana = env.ana();
  jumanpp::rnn::mikolov::MikolovModelReader mrr;
  REQUIRE_OK(mrr.open("rnn/testlm"));
  REQUIRE_OK(mrr.parse());

  a::ScorerDef scorerDef{};
  scorerDef.scoreWeights.push_back(1.0f);
  scorerDef.scoreWeights.push_back(1.0f);
  a::rnn::RnnHolder rnnHolder;
  scorerDef.feature = &env.perceptron;
  REQUIRE_OK(rnnHolder.init({}, mrr, env.jppEnv.coreHolder()->dic(), "a"));
  scorerDef.others.push_back(&rnnHolder);
  REQUIRE_OK(ana.initScorers(scorerDef));
  ana.resetForInput("newsanapple");
  REQUIRE_OK(ana.prepareNodeSeeds());
  REQUIRE_OK(ana.buildLattice());
  REQUIRE_OK(ana.bootstrapAnalysis());
  REQUIRE_OK(ana.computeScores(&scorerDef));
  CHECK(ana.lattice()->createdBoundaryCount() == 14);
  CHECK(!std::isnan(
      ana.lattice()->boundary(13)->starts()->beamData().at(0).totalScore));
  // env.dumpTrainers("/tmp/jpp");
}

TEST_CASE("RNN holder serializes and deserializes and has the same score") {
  RnnScorerEnv env{
      "newsan,12\nn,14\newsan,13\nnew,1\nnews,2\nsan,3\na,4\nan,5\n"
      "apple,6\news,7\nne,20\npple,8\nwsa,9\np,10\nle,11\n"};
  auto& ana = env.ana();
  jumanpp::rnn::mikolov::MikolovModelReader mrr;
  REQUIRE_OK(mrr.open("rnn/testlm"));
  REQUIRE_OK(mrr.parse());
  a::rnn::RnnHolder rnnHolder;
  REQUIRE_OK(rnnHolder.init({}, mrr, env.jppEnv.coreHolder()->dic(), "a"));

  core::model::ModelInfo modelInfo{};
  REQUIRE_OK(rnnHolder.makeInfo(&modelInfo));
  a::rnn::RnnHolder rnnHolder2;
  REQUIRE_OK(rnnHolder2.load(modelInfo));

  a::ScorerDef scorerDef1{};
  scorerDef1.scoreWeights.push_back(1.0f);
  scorerDef1.scoreWeights.push_back(1.0f);
  scorerDef1.feature = &env.perceptron;
  scorerDef1.others.push_back(&rnnHolder);
  a::AnalyzerImpl impl1{env.jppEnv.coreHolder(), env.scoreCfg, env.anaCfg};
  REQUIRE_OK(impl1.initScorers(scorerDef1));

  a::ScorerDef scorerDef2{};
  scorerDef2.scoreWeights.push_back(1.0f);
  scorerDef2.scoreWeights.push_back(1.0f);
  scorerDef2.feature = &env.perceptron;
  scorerDef2.others.push_back(&rnnHolder2);
  a::AnalyzerImpl impl2{env.jppEnv.coreHolder(), env.scoreCfg, env.anaCfg};
  REQUIRE_OK(impl2.initScorers(scorerDef2));

  impl1.resetForInput("newsanapple");
  REQUIRE_OK(impl1.prepareNodeSeeds());
  REQUIRE_OK(impl1.buildLattice());
  REQUIRE_OK(impl1.bootstrapAnalysis());
  REQUIRE_OK(impl1.computeScores(&scorerDef1));

  impl2.resetForInput("newsanapple");
  REQUIRE_OK(impl2.prepareNodeSeeds());
  REQUIRE_OK(impl2.buildLattice());
  REQUIRE_OK(impl2.bootstrapAnalysis());
  REQUIRE_OK(impl2.computeScores(&scorerDef1));

  CHECK(impl1.lattice()->boundary(13)->starts()->beamData().at(0).totalScore ==
        impl2.lattice()->boundary(13)->starts()->beamData().at(0).totalScore);
}

TEST_CASE("RNN holder serializes and deserializes") {
  RnnScorerEnv env{
      "newsan,12\nn,14\newsan,13\nnew,1\nnews,2\nsan,3\na,4\nan,5\n"
      "apple,6\news,7\nne,20\npple,8\nwsa,9\np,10\nle,11\n"};
  auto& ana = env.ana();
  jumanpp::rnn::mikolov::MikolovModelReader mrr;
  REQUIRE_OK(mrr.open("rnn/testlm"));
  REQUIRE_OK(mrr.parse());
  a::rnn::RnnHolder rnnHolder;
  REQUIRE_OK(rnnHolder.init({}, mrr, env.jppEnv.coreHolder()->dic(), "a"));

  core::model::ModelInfo modelInfo{};
  REQUIRE_OK(rnnHolder.makeInfo(&modelInfo));
  a::rnn::RnnHolder rnnHolder2;
  REQUIRE_OK(rnnHolder2.load(modelInfo));
}

TEST_CASE("RnnIdAdder correctly handles unks") {
  util::memory::Manager mgr{4096};
  auto alloc = mgr.core();
  a::rnn::RnnIdContainer ctr{alloc.get()};
  a::rnn::RnnIdAdder adder2{&ctr, 1};
  adder2.add(-1, 2);
  adder2.finish();
  a::rnn::RnnIdAdder guard{&ctr, 2};
  checkItems(ctr, 0, 1);
}

TEST_CASE("RnnIdAdder correctly handles unks in end") {
  util::memory::Manager mgr{4096};
  auto alloc = mgr.core();
  a::rnn::RnnIdContainer ctr{alloc.get()};
  a::rnn::RnnIdAdder adder2{&ctr, 1};
  adder2.add(1, 2);
  adder2.add(-1, 2);
  adder2.finish();
  a::rnn::RnnIdAdder guard{&ctr, 2};
  checkItems(ctr, 0, 2, 2);
  CHECK(ctr.atBoundary(0).span(0).normal);
  CHECK(ctr.atBoundary(0).span(0).length == 1);
  CHECK(ctr.atBoundary(0).span(0).start == 0);
  CHECK(ctr.atBoundary(0).ids().at(0) == 1);
  CHECK(ctr.atBoundary(0).ids().at(1) == -1);
}

TEST_CASE("RnnIdAdder correctly handles unks in start") {
  util::memory::Manager mgr{4096};
  auto alloc = mgr.core();
  a::rnn::RnnIdContainer ctr{alloc.get()};
  a::rnn::RnnIdAdder adder2{&ctr, 1};
  adder2.add(-1, 2);
  adder2.add(1, 2);
  adder2.finish();
  a::rnn::RnnIdAdder guard{&ctr, 2};
  checkItems(ctr, 0, 2, 2);
  CHECK(ctr.atBoundary(0).span(1).normal);
  CHECK(ctr.atBoundary(0).span(1).length == 1);
  CHECK(ctr.atBoundary(0).span(1).start == 1);
  CHECK(ctr.atBoundary(0).ids().at(0) == -1);
  CHECK(ctr.atBoundary(0).ids().at(1) == 1);
}

TEST_CASE("RnnIdAdder correctly handles unks in middle") {
  util::memory::Manager mgr{4096};
  auto alloc = mgr.core();
  a::rnn::RnnIdContainer ctr{alloc.get()};
  a::rnn::RnnIdAdder adder2{&ctr, 1};
  adder2.add(1, 2);
  adder2.add(-1, 2);
  adder2.add(1, 2);
  adder2.finish();
  a::rnn::RnnIdAdder guard{&ctr, 2};
  checkItems(ctr, 0, 3, 3);
  CHECK(ctr.atBoundary(0).span(0).normal);
  CHECK_FALSE(ctr.atBoundary(0).span(1).normal);
  CHECK(ctr.atBoundary(0).span(2).normal);
  CHECK(ctr.atBoundary(0).span(0).length == 1);
  CHECK(ctr.atBoundary(0).span(2).length == 1);
  CHECK(ctr.atBoundary(0).span(0).start == 0);
  CHECK(ctr.atBoundary(0).ids().at(0) == 1);
  CHECK(ctr.atBoundary(0).ids().at(1) == -1);
  CHECK(ctr.atBoundary(0).ids().at(2) == 1);
}

TEST_CASE("RnnIdAdder correctly handles unks (2) in middle") {
  util::memory::Manager mgr{4096};
  auto alloc = mgr.core();
  a::rnn::RnnIdContainer ctr{alloc.get()};
  a::rnn::RnnIdAdder adder1{&ctr, 0};
  adder1.add(3, 2);
  adder1.add(3, 2);
  adder1.finish();
  a::rnn::RnnIdAdder adder2{&ctr, 1};
  adder2.add(1, 2);
  adder2.add(-1, 12);
  adder2.add(1, 2);
  adder2.add(5, 2);
  adder2.finish();
  a::rnn::RnnIdAdder guard{&ctr, 2};
  checkItems(ctr, 0, 2, 1);
  checkItems(ctr, 1, 4, 3);
  CHECK(ctr.atBoundary(1).span(0).normal);
  CHECK(ctr.atBoundary(1).span(0).length == 1);
  CHECK(ctr.atBoundary(1).span(0).start == 0);
  CHECK(ctr.atBoundary(1).span(0).length == 1);
  CHECK(ctr.atBoundary(1).span(1).normal == false);
  CHECK(ctr.atBoundary(1).span(1).start == 1);
  CHECK(ctr.atBoundary(1).span(1).length == 12);
  CHECK(ctr.atBoundary(1).span(2).normal);
  CHECK(ctr.atBoundary(1).span(2).start == 2);
  CHECK(ctr.atBoundary(1).span(2).length == 2);
  CHECK(ctr.atBoundary(1).ids().at(0) == 1);
  CHECK(ctr.atBoundary(1).ids().at(1) == -1);
  CHECK(ctr.atBoundary(1).ids().at(2) == 1);
  CHECK(ctr.atBoundary(1).ids().at(3) == 5);
}