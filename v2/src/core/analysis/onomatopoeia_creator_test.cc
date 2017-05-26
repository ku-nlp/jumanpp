//
// Created by nakao on 2017/05/24.
//
#include "onomatopoeia_creator.h"
#include "testing/test_analyzer.h"

using namespace jumanpp::core::analysis;
using namespace jumanpp::core::spec;
using namespace jumanpp::core::dic;
using namespace jumanpp::testing;
using namespace jumanpp;

namespace {
class OnomatopoeiaTestEnv {
  TestEnv tenv;
  StringField fld1;
  StringField fld2;

 public:
  OnomatopoeiaTestEnv(StringPiece csvData) {
    tenv.spec([](dsl::ModelSpecBuilder& specBldr) {
      auto& a = specBldr.field(1, "f1").strings().trieIndex();
      specBldr.field(2, "f2").strings();
      specBldr.unk("onomatopoeia", 1)
          .onomatopoeia(chars::CharacterClass::FAMILY_KANA)
          .outputTo({a});
    });
    CHECK(tenv.saveLoad.unkCreators.size() == 1);
    tenv.importDic(csvData);
    REQUIRE_OK(tenv.analyzer->output().stringField("f1", &fld1));
    REQUIRE_OK(tenv.analyzer->output().stringField("f2", &fld2));
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
          auto s1 = fld1[walker];
          auto s2 = fld2[walker];
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
        WARN("NODE:" << fld1[walker] << "||" << fld2[walker]);
      }
    }
  }

  ~OnomatopoeiaTestEnv() {
    if (!Catch::getResultCapture().getLastResult()->isOk()) {
      printAll();
    }
  }
};
}  // namespace

TEST_CASE("onomatopoeic unk nodes are spawned") {
  OnomatopoeiaTestEnv env{"x,l1\nほげ,l2\n"};
  env.analyze("ほげほげニャンニャンﾁｸﾀｸﾁｸﾀｸ");
  CHECK(env.contains("ほげ", 0, "l2"));
  CHECK(env.contains("ほげ", 2, "l2"));
  CHECK(env.contains("ほげほげ", 0, "l1"));
  CHECK(env.contains("ニャンニャン", 4, "l1"));
  CHECK(env.contains("ﾁｸﾀｸﾁｸﾀｸ", 10, "l1"));
  CHECK(env.numNodeSeeds() == 5);
}

TEST_CASE("onomatopoeic unks do not go over different classes") {
  OnomatopoeiaTestEnv env{"x,l1\nhoge,l2\n"};
  env.analyze("ﾎｹﾞホゲホゲほげ");
  CHECK(env.contains("ホゲホゲ", 3, "l1"));
  CHECK(env.numNodeSeeds() == 1);
}

TEST_CASE("onomatopoeia is less than or equal to 8 characters") {
  OnomatopoeiaTestEnv env{"x,l1\nhoge,l2\n"};
  env.analyze("あいうえおあいうえお");
  CHECK(env.numNodeSeeds() == 0);
}

TEST_CASE("onomatopoeia including hankaku dakuten and handakuten are valid") {
  OnomatopoeiaTestEnv env{"x,l1\nhoge,l2\n"};
  env.analyze("ﾎｹﾞﾎｹﾞﾎﾟﾖﾎﾟﾖｼﾞｮﾊﾞｼﾞｮﾊﾞ");
  CHECK(env.contains("ﾎｹﾞﾎｹﾞ", 0, "l1"));
  CHECK(env.contains("ｹﾞﾎｹﾞﾎ", 1, "l1"));
  CHECK(env.contains("ﾎﾟﾖﾎﾟﾖ", 6, "l1"));
  // "ｼﾞｮﾊﾞｼﾞｮﾊﾞ" is more than 8 characters because "ﾞ" is counted as 1
  // character.
  CHECK_FALSE(env.contains("ｼﾞｮﾊﾞｼﾞｮﾊﾞ", 12, "l1"));
  CHECK(env.numNodeSeeds() == 3);
}
