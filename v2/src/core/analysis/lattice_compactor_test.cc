//
// Created by Arseny Tolmachev on 2017/03/06.
//

#include "testing/test_analyzer.h"

using namespace jumanpp::core::analysis;
using namespace jumanpp::core::spec;
using namespace jumanpp::core::dic;
using namespace jumanpp::testing;
using namespace jumanpp;

namespace {
class LatticeCompactorTestEnv {
  TestEnv tenv;

 public:
  StringField flda;
  StringField fldb;
  StringField fldc;

 public:
  LatticeCompactorTestEnv(StringPiece csvData) {
    tenv.spec([](dsl::ModelSpecBuilder& specBldr) {
      auto& a = specBldr.field(1, "a").strings().trieIndex();
      auto& b = specBldr.field(2, "b").strings();
      specBldr.field(3, "c").strings();
      specBldr.unk("chars", 1)
          .chunking(chars::CharacterClass::KATAKANA)
          .outputTo({a});
      specBldr.unigram({a, b});
    });
    CHECK(tenv.saveLoad.unkCreators.size() == 1);
    tenv.importDic(csvData);
    REQUIRE_OK(tenv.analyzer->output().stringField("a", &flda));
    REQUIRE_OK(tenv.analyzer->output().stringField("b", &fldb));
    REQUIRE_OK(tenv.analyzer->output().stringField("c", &fldc));
    ScorerDef scfg{};
    scfg.scoreWeights.push_back(1.0f);
    REQUIRE_OK(tenv.analyzer->initScorers(scfg));
  }

  void analyze(StringPiece str) {
    CAPTURE(str);
    CHECK_OK(tenv.analyzer->resetForInput(str));
    CHECK_OK(tenv.analyzer->prepareNodeSeeds());
    CHECK_OK(tenv.analyzer->buildLattice());
  }

  Lattice* lattice() { return tenv.analyzer->lattice(); }

  const OutputManager& output() { return tenv.analyzer->output(); }

  void printAll() {
    auto& output = tenv.analyzer->output();
    auto walker = output.nodeWalker();
    auto seeds = tenv.analyzer->latticeBuilder().seeds();

    for (auto& seed : seeds) {
      CHECK(output.locate(seed.entryPtr, &walker));
      while (walker.next()) {
        WARN("NODE:[" << seed.codepointStart << "," << seed.codepointEnd << "] "
                      << flda[walker] << "||" << fldb[walker]);
      }
    }
  }

  ~LatticeCompactorTestEnv() {
    if (!Catch::getResultCapture().getLastResult()->isOk()) {
      printAll();
    }
  }
};
}  // namespace

TEST_CASE("compactor works") {
  LatticeCompactorTestEnv env{
      "KANA,0,x\nすもも,1,x\nすもも,1,y\nもも,2,x\nもも,2,z\nも,3,x\nうち,4,"
      "x\nの,5,x"};
  env.analyze("すもももももももものうち");
  CHECK(1 == env.lattice()->boundary(2)->localNodeCount());
  CHECK(2 == env.lattice()->boundary(3)->localNodeCount());
  auto& output = env.output();
  auto walker = output.nodeWalker();
  CHECK(output.locate(
      env.lattice()->boundary(3)->starts()->nodeInfo().at(1).entryPtr(),
      &walker));
  CHECK(walker.remaining() == 2);
  CHECK(walker.next());
  CHECK(env.fldc[walker] == "x");
  CHECK(walker.next());
  CHECK(env.fldc[walker] == "z");
  CHECK_FALSE(walker.next());
}