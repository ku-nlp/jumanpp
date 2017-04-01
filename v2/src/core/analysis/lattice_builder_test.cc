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

 public:
  LatticeBldrTestEnv(StringPiece csvData) {
    tenv.spec([](dsl::ModelSpecBuilder& specBldr) {
      auto& a = specBldr.field(1, "a").strings().trieIndex();
      specBldr.field(2, "b").strings();
      specBldr.unk("chars", 1)
          .chunking(chars::CharacterClass::KATAKANA)
          .outputTo({a});
    });
    CHECK(tenv.saveLoad.unkCreators.size() == 1);
    tenv.importDic(csvData);
    REQUIRE_OK(tenv.analyzer->output().stringField("a", &flda));
    REQUIRE_OK(tenv.analyzer->output().stringField("b", &fldb));
  }

  void analyze(StringPiece str) {
    CAPTURE(str);
    CHECK_OK(tenv.analyzer->resetForInput(str));
    CHECK_OK(tenv.analyzer->makeNodeSeedsFromDic());
    CHECK_OK(tenv.analyzer->makeUnkNodes1());
  }

  bool isConnected() { return tenv.analyzer->checkLatticeConnectivity(); }

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
    if (!Catch::getResultCapture().getLastResult()->isOk()) {
      printAll();
    }
  }
};
}  // namespace

TEST_CASE("connected lattice") {
  LatticeBldrTestEnv env{"KANA,0\nすもも,1\nもも,2\nも,3\nうち,4\nの,5"};
  env.analyze("すもももももももものうち");
  CHECK(env.isConnected());
  env.analyze("すもももすいかももものうち");
  CHECK_FALSE(env.isConnected());
}