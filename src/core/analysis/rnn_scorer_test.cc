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
  std::vector<float> features_{-0.1f, -0.15f, 0.15f, 0.1f};
  util::Lazy<core::analysis::AnalyzerImpl> anaImpl;
  core::analysis::AnalyzerConfig anaCfg{};
  core::ScoringConfig scoreCfg;
  core::analysis::HashedFeaturePerceptron perceptron{features_};

  RnnScorerEnv(StringPiece dic) {
    env.spec([](core::spec::dsl::ModelSpecBuilder& sb) {
      auto a = sb.field(1, "a").strings().trieIndex();
      auto b = sb.field(2, "b").strings();

      sb.trigram({a, b}, {a, b}, {a, b});
      sb.trigram({a}, {a}, {a});
      sb.bigram({a}, {a});
    });
    env.saveDic(dic);
    env.loadEnv(&jppEnv);
    anaCfg.globalBeamSize = 50;
    anaCfg.rightGbeamSize = 5;
    anaCfg.rightGbeamCheck = 2;
    jppEnv.setBeamSize(5);
    jppEnv.setGlobalBeam(50, 1, 5);
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

TEST_CASE("RNN computes scores") {
  RnnScorerEnv env{"a,1\nb,2\nc,3"};
  auto& ana = env.ana();
  jumanpp::rnn::mikolov::MikolovModelReader mrr;
  REQUIRE_OK(mrr.open("rnn/testlm"));
  REQUIRE_OK(mrr.parse());

  a::ScorerDef scorerDef{};
  scorerDef.scoreWeights.push_back(1.0f);
  scorerDef.scoreWeights.push_back(1.0f);
  scorerDef.feature = &env.perceptron;
  a::RnnScorerGbeamFactory rnnHolder;
  core::analysis::rnn::RnnInferenceConfig ric;
  ric.rnnFields = {"a"};
  ric.fieldSeparator = ",";
  REQUIRE_OK(rnnHolder.make("rnn/testlm", env.jppEnv.coreHolder()->dic(), ric));
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
  scorerDef.feature = &env.perceptron;
  a::RnnScorerGbeamFactory rnnHolder;
  core::analysis::rnn::RnnInferenceConfig ric;
  ric.rnnFields = {"a"};
  ric.fieldSeparator = ",";
  ric.unkConstantTerm = -15.0f;
  ric.unkLengthPenalty = -5.0f;
  REQUIRE_OK(rnnHolder.make("rnn/testlm", env.jppEnv.coreHolder()->dic(), ric));
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
  // env.dumpTrainers("/tmp/jpp-dbg");
}

TEST_CASE("RNN computes scores with and collapsibles") {
  RnnScorerEnv env{
      "news,1\nnews,2\nnew,1\nnew,2\nsan,1\nsan,2\n"
      "an,4\nan,5\napple,1\napple,2\n"
      "a,4\na,5\nnap,1\nnap,2\np,10\nle,11\nple,10\nple,11\n"};
  auto& ana = env.ana();
  jumanpp::rnn::mikolov::MikolovModelReader mrr;
  REQUIRE_OK(mrr.open("rnn/testlm"));
  REQUIRE_OK(mrr.parse());

  a::ScorerDef scorerDef{};
  scorerDef.scoreWeights.push_back(1.0f);
  scorerDef.scoreWeights.push_back(1.0f);
  scorerDef.feature = &env.perceptron;
  a::RnnScorerGbeamFactory rnnHolder;
  core::analysis::rnn::RnnInferenceConfig ric;
  ric.rnnFields = {"a"};
  ric.fieldSeparator = ",";
  ric.unkConstantTerm = -15.0f;
  ric.unkLengthPenalty = -5.0f;
  REQUIRE_OK(rnnHolder.make("rnn/testlm", env.jppEnv.coreHolder()->dic(), ric));
  scorerDef.others.push_back(&rnnHolder);
  REQUIRE_OK(ana.initScorers(scorerDef));
  ana.resetForInput("newsanapple");
  REQUIRE_OK(ana.prepareNodeSeeds());
  REQUIRE_OK(ana.buildLattice());
  REQUIRE_OK(ana.bootstrapAnalysis());
  for (auto bndIdx = 2; bndIdx < ana.lattice()->createdBoundaryCount();
       ++bndIdx) {
    auto bnd = ana.lattice()->boundary(bndIdx);
    auto sc = bnd->scores();
    for (int i = 0; i < bnd->localNodeCount(); ++i) {
      sc->nodeScores(i).fill(1e30f);
    }
  }
  REQUIRE_OK(ana.computeScores(&scorerDef));
  CHECK(ana.lattice()->createdBoundaryCount() == 14);
  auto topScore =
      ana.lattice()->boundary(13)->starts()->beamData().at(0).totalScore;
  CHECK(!std::isnan(topScore));
  CHECK(topScore < 1e10f);
  // env.dumpTrainers("/tmp/jpp-dbg");
}

TEST_CASE("RNN holder serializes and deserializes and has the same score") {
  RnnScorerEnv env{
      "newsan,12\nn,14\newsan,13\nnew,1\nnews,2\nsan,3\na,4\nan,5\n"
      "apple,6\news,7\nne,20\npple,8\nwsa,9\np,10\nle,11\n"};
  auto& ana = env.ana();
  jumanpp::rnn::mikolov::MikolovModelReader mrr;
  REQUIRE_OK(mrr.open("rnn/testlm"));
  REQUIRE_OK(mrr.parse());
  a::RnnScorerGbeamFactory rnnHolder;
  core::analysis::rnn::RnnInferenceConfig ric;
  ric.rnnFields = {"a"};
  ric.fieldSeparator = ",";
  REQUIRE_OK(rnnHolder.make("rnn/testlm", env.jppEnv.coreHolder()->dic(), ric));

  core::model::ModelInfo modelInfo{};
  REQUIRE_OK(rnnHolder.makeInfo(&modelInfo, EMPTY_SP));
  a::RnnScorerGbeamFactory rnnHolder2;
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
  a::RnnScorerGbeamFactory rnnHolder;
  core::analysis::rnn::RnnInferenceConfig ric;
  ric.rnnFields = {"a"};
  ric.fieldSeparator = ",";
  REQUIRE_OK(rnnHolder.make("rnn/testlm", env.jppEnv.coreHolder()->dic(), ric));

  core::model::ModelInfo modelInfo{};
  REQUIRE_OK(rnnHolder.makeInfo(&modelInfo, EMPTY_SP));
  a::RnnScorerGbeamFactory rnnHolder2;
  REQUIRE_OK(rnnHolder2.load(modelInfo));
}

