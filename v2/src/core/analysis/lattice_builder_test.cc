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
class LatticeBldrTestEnv {
  TestEnv tenv;
  StringField flda;
  StringField fldb;
  ScorerDef scorerDef;

 public:
  LatticeBldrTestEnv(StringPiece csvData) {
    tenv.spec([](dsl::ModelSpecBuilder& specBldr) {
      auto& a = specBldr.field(1, "a").strings().trieIndex();
      specBldr.field(2, "b").strings();
      specBldr.unk("chars", 1)
          .chunking(chars::CharacterClass::KATAKANA)
          .outputTo({a});
    });
    CHECK(tenv.originalSpec.unkCreators.size() == 1);
    tenv.importDic(csvData);
    REQUIRE_OK(tenv.analyzer->output().stringField("a", &flda));
    REQUIRE_OK(tenv.analyzer->output().stringField("b", &fldb));
    scorerDef.scoreWeights.push_back(1.0f);
    tenv.analyzer->initScorers(scorerDef);
  }

  void analyze(StringPiece str, bool doBuild) {
    CAPTURE(str);
    REQUIRE_OK(tenv.analyzer->resetForInput(str));
    Status s = tenv.analyzer->prepareNodeSeeds();
    if (doBuild) {
      REQUIRE_OK(s);
      REQUIRE_OK(tenv.analyzer->buildLattice());
    }
  }

  bool isConnected() { return tenv.analyzer->checkLatticeConnectivity(); }

  Lattice* lat() { return tenv.analyzer.get()->lattice(); }

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

  ~LatticeBldrTestEnv() {
    if (!Catch::getResultCapture().lastAssertionPassed()) {
      printAll();
    }
  }
};
}  // namespace

TEST_CASE("connected lattice") {
  LatticeBldrTestEnv env{"KANA,0\nすもも,1\nもも,2\nも,3\nうち,4\nの,5"};
  env.analyze("すもももももももものうち", true);
  CHECK(env.isConnected());
  auto codepts =
      env.lat()->boundary(2)->starts()->nodeInfo().at(0).numCodepoints();
  CHECK(codepts == 3);
}

TEST_CASE("disconnected lattice") {
  LatticeBldrTestEnv env{"KANA,0\nすもも,1\nもも,2\nも,3\nうち,4\nの,5"};
  env.analyze("すもももすいかももものうち", false);
  CHECK_FALSE(env.isConnected());
}

TEST_CASE("disconnected lattice with overlapping tokens") {
  LatticeBldrTestEnv env{"KANA,0\nみあげ,1\nげる,2\nあげ,3\n"};
  env.analyze("みあげる", false);
  CHECK_FALSE(env.isConnected());
}