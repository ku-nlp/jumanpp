//
// Created by Arseny Tolmachev on 2017/11/17.
//

#include "rnn_id_resolver.h"
#include "rnn/mikolov_rnn.h"
#include "testing/test_analyzer.h"

using namespace jumanpp;

namespace {
class RnnIdTestEnv {
  testing::TestEnv env;
  rnn::mikolov::MikolovModelReader rnn;
  core::analysis::HashedFeaturePerceptron hfp{{0.1f, 0.2f, 0.3f, 0.4f}};
  core::analysis::ScorerDef sdef;

 public:
  RnnIdTestEnv(StringPiece dic) {
    env.beamSize = 3;
    env.spec([](core::spec::dsl::ModelSpecBuilder& bldr) {
      auto& a = bldr.field(1, "a").strings().trieIndex();
      auto& b = bldr.field(2, "b").strings();
      auto& c = bldr.field(3, "c").strings();
      bldr.unigram({a, b});
    });
    env.importDic(dic);
    REQUIRE(rnn.open("rnn/testlm"));
    REQUIRE(rnn.parse());
    sdef.feature = &hfp;
    sdef.scoreWeights.push_back(1.0f);
    REQUIRE(env.analyzer->setGlobalBeam(3, 1, 3));
    REQUIRE(env.analyzer->initScorers(sdef));
  }

  const rnn::mikolov::MikolovModelReader& rnnRdr() const { return rnn; }
  const core::CoreHolder& core() const { return *env.core.get(); }
  void analyze(StringPiece str) {
    auto& an = *env.analyzer.get();
    REQUIRE(an.resetForInput(str));
    REQUIRE(an.prepareNodeSeeds());
    REQUIRE(an.buildLattice());
    REQUIRE(an.bootstrapAnalysis());
    REQUIRE(an.computeScores(&sdef));
  }

  core::analysis::AnalyzerImpl* analyzer() { return env.analyzer.get(); }
};
}  // namespace

TEST_CASE("pointer to beam is equal to path pointer") {
  core::analysis::ConnectionBeamElement cptr;
  CHECK(std::is_standard_layout<core::analysis::ConnectionBeamElement>::value);
  CHECK(reinterpret_cast<const char*>(&cptr) ==
        reinterpret_cast<const char*>(&cptr.ptr));
}

TEST_CASE("can build a rnn") {
  StringPiece dic{
      "test,a,\nte,a,\nno,a,\ngust,a,\nst,a,\nnow,a,\nhere,a,\nnowhere,a,"
      "\nwhere,a,\n"};
  RnnIdTestEnv env{dic};
  util::memory::Manager mgr{512 * 1024};
  auto alloc = mgr.core();
  core::analysis::rnn::RnnIdContainer2 cont{alloc.get()};
  core::analysis::rnn::RnnIdResolver2 res;
  core::analysis::rnn::RnnInferenceConfig ric;
  ric.rnnFields = {"a"};
  REQUIRE(res.build(env.core().dic(), ric, env.rnnRdr().words()));
  CHECK(res.unkId() == 2);
  env.analyze("gusttestnowhere");
  cont.reset(env.analyzer()->lattice()->createdBoundaryCount(), 3);
  CHECK(res.resolveIdsAtGbeam(&cont, env.analyzer()->lattice(),
                              env.analyzer()->extraNodesContext()));
  CHECK(cont.rnnBoundary(1).nodeCnt == 1);
  CHECK(cont.rnnBoundary(2).nodeCnt == 1);
  CHECK(cont.rnnBoundary(17).nodeCnt == 3);
}