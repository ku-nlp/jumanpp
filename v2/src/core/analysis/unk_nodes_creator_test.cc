//
// Created by Arseny Tolmachev on 2017/03/01.
//
#include "testing/test_analyzer.h"

using namespace jumanpp::core::analysis;
using namespace jumanpp::core::spec;
using namespace jumanpp::core::dic;
using namespace jumanpp::testing;
using namespace jumanpp;

namespace {
class UnkNodeTestEnv {
  TestEnv tenv;
  StringField flda;
  StringField fldb;

 public:
  UnkNodeTestEnv(StringPiece csvData) {
    tenv.spec([](dsl::ModelSpecBuilder& specBldr) {
      auto& a = specBldr.field(1, "a").strings().trieIndex();
      specBldr.field(2, "b").strings();
      specBldr.unk("chars", 1)
          .chunking(chars::CharacterClass::ALPH)
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

  size_t numNodeSeeds() const {
    return tenv.analyzer->latticeBuilder().seeds().size();
  }

  bool contains(StringPiece str, i32 start, StringPiece strb) {
    CAPTURE(str);
    CAPTURE(start);
    auto& output = tenv.analyzer->output();
    auto walker = output.nodeWalker();

    auto seeds = tenv.analyzer->latticeBuilder().seeds();
    std::vector<chars::InputCodepoint> cp;
    chars::preprocessRawData(str, &cp);
    i32 end = start + cp.size();
    for (auto& seed : seeds) {
      if (seed.codepointStart == start && seed.codepointEnd == end) {
        CHECK(output.locate(seed.entryPtr, &walker));
        while (walker.next()) {
          auto s1 = flda[walker];
          auto s2 = fldb[walker];
          if (s1 == str && s2 == strb) {
            return true;
          }
        }
      }
    }
    return false;
  }

  void printAll() {
    auto& output = tenv.analyzer->output();
    auto walker = output.nodeWalker();
    auto seeds = tenv.analyzer->latticeBuilder().seeds();

    for (auto& seed : seeds) {
      CHECK(output.locate(seed.entryPtr, &walker));
      while (walker.next()) {
        WARN("NODE:" << flda[walker] << "||" << fldb[walker]);
      }
    }
  }

  ~UnkNodeTestEnv() {
    if (!Catch::getResultCapture().lastAssertionPassed()) {
      printAll();
    }
  }
};
}  // namespace

TEST_CASE("unk nodes are spawned") {
  UnkNodeTestEnv env{"x,a\nxa,b\n"};
  env.analyze("dxa");
  CHECK(env.contains("xa", 1, "b"));
  CHECK(env.contains("d", 0, "a"));
  CHECK(env.contains("dx", 0, "a"));
  CHECK(env.contains("dxa", 0, "a"));
  CHECK(env.contains("x", 1, "a"));
  CHECK(env.contains("a", 2, "a"));
  CHECK(env.numNodeSeeds() == 6);
}

TEST_CASE("chunked unks do not go over different classes") {
  UnkNodeTestEnv env{"x,a\ndx,b\n1,c"};
  env.analyze("dx1a");
  CHECK(env.contains("d", 0, "a"));
  CHECK(env.contains("a", 3, "a"));
  CHECK(env.contains("x", 1, "a"));
  CHECK(env.contains("dx", 0, "b"));
  CHECK(env.contains("1", 2, "c"));
  CHECK(env.numNodeSeeds() == 5);
}