//
// Created by morita on 2017/07/07.
//

#include "testing/test_analyzer.h"

using namespace jumanpp::core::analysis;
using namespace jumanpp::core::spec;
using namespace jumanpp::core::dic;
using namespace jumanpp::testing;
using namespace jumanpp;

namespace {
class NumericTestEnv {
  TestEnv tenv;
  StringField fld1;
  StringField fld2;

 public:
  NumericTestEnv(StringPiece csvData) {
    tenv.spec([](dsl::ModelSpecBuilder& specBldr) {
      auto& a = specBldr.field(1, "f1").strings().trieIndex();
      specBldr.field(2, "f2").strings();
      specBldr.unk("numeric", 1)
          .numeric(chars::CharacterClass::FAMILY_DIGITS)  // 仕様を要検討
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
        WARN("NODE (" << seed.codepointStart << ", " << seed.codepointEnd
                      << "):" << fld1[walker] << "||" << fld2[walker]);
      }
    }
  }

  ~NumericTestEnv() {
    if (!Catch::getResultCapture().getLastResult()->isOk()) {
      printAll();
    }
  }
};
}  // namespace

TEST_CASE("single numeric unk node is spawned") {
  NumericTestEnv env{"x,l1\nほげ,l2\n"};
  env.analyze("１");
  CHECK(env.contains("１", 0, "l1"));
  CHECK(env.numNodeSeeds() == 1);
}

TEST_CASE("unk node is not spawned when there is no number") {
  NumericTestEnv env{"x,l1\nほげ,l2\n"};
  env.analyze("隣のほげはよく柿食うほげだ");
  CHECK(env.numNodeSeeds() == 2);
}

TEST_CASE("accept a numeric unk node consisted by prefix and suffix") {
  NumericTestEnv env{"x,l1\nほげ,l2\n"};
  env.analyze("数ミリ");
  CHECK(env.contains("数ミリ", 0, "l1"));
  CHECK(env.numNodeSeeds() == 1);
}

TEST_CASE("do not accept unk node consisted by only prefix or suffix") {
  NumericTestEnv env{"x,l1\nほげ,l2\n"};
  env.analyze("ほげミリ");
  CHECK(!env.contains("ミリ", 2, "l1"));
  CHECK(env.numNodeSeeds() == 1);
}

TEST_CASE("accept a numeric unk node including prefix and suffix") {
  NumericTestEnv env{"x,l1\nほげ,l2\n"};
  env.analyze("ほげ数ミリほげ数十ミリ");
  CHECK(env.contains("数十ミリ", 7, "l1"));
  CHECK(env.numNodeSeeds() == 5);
}

TEST_CASE("make a numeric including prefix unk node") {
  NumericTestEnv env{"x,l1\nほげ,l2\n"};
  env.analyze("数十");
  CHECK(env.contains("数十", 0, "l1"));
  CHECK(env.contains("十", 1, "l1"));
  CHECK(env.numNodeSeeds() == 2);
}

TEST_CASE(
    "do not make a numeric including prefix next to non DIGIT characters unk "
    "node") {
  NumericTestEnv env{"x,l1\nほげ,l2\n"};
  env.analyze("数５");
  CHECK(!env.contains("数", 0, "l1"));
  CHECK(env.contains("５", 1, "l1"));
  CHECK(env.numNodeSeeds() == 1);
}

TEST_CASE("numeric unk nodes after other words are spawned") {
  NumericTestEnv env{"x,l1\nほげ,l2\n"};
  env.analyze("ほげ１２３");
  CHECK(env.contains("１２３", 2, "l1"));
  CHECK(env.contains("２３", 3, "l1"));
  CHECK(env.contains("３", 4, "l1"));
  CHECK(env.numNodeSeeds() == 4);
}

TEST_CASE("pulural numeric unk nodes are spawned") {
  NumericTestEnv env{"x,l1\nほげ,l2\n"};
  env.analyze("ほげ４５６ほげ４３１");
  CHECK(env.contains("４５６", 2, "l1"));
  CHECK(env.contains("５６", 3, "l1"));
  CHECK(env.contains("６", 4, "l1"));
  CHECK(env.contains("４３１", 7, "l1"));
  CHECK(env.contains("３１", 8, "l1"));
  CHECK(env.contains("１", 9, "l1"));
  CHECK(env.numNodeSeeds() == 8);
}

TEST_CASE("exceptional numeric unk nodes are spawned") {
  NumericTestEnv env{"x,l1\nほげ,l2\n"};
  env.analyze("ほげ４５６キロ");
  CHECK(env.contains("４５６キロ", 2, "l1"));
  CHECK(env.contains("５６キロ", 3, "l1"));
  CHECK(env.contains("６キロ", 4, "l1"));
  CHECK(env.numNodeSeeds() == 4);
}

TEST_CASE("two or more continuous suffixes are not accepted") {
  NumericTestEnv env{"x,l1\nほげ,l2\n"};
  env.analyze("４５６キロミリ");
  CHECK(env.contains("４５６キロ", 0, "l1"));
  CHECK(!env.contains("４５６キロミリ", 0, "l1"));
  CHECK(env.numNodeSeeds() == 3);
}

TEST_CASE("numeric unk nodes including period are spawned") {
  NumericTestEnv env{"x,l1\nほげ,l2\n"};
  env.analyze("ほげ４．５６");
  CHECK(env.contains("４．５６", 2, "l1"));
  CHECK(!env.contains("．５６", 3, "l1"));
  CHECK(env.contains("５６", 4, "l1"));
  CHECK(env.contains("６", 5, "l1"));
  CHECK(env.numNodeSeeds() == 4);
}

TEST_CASE("single numeric unk nodes including period are spawned") {
  NumericTestEnv env{"x,l1\nほげ,l2\n"};
  env.analyze("４．５６");
  CHECK(env.contains("４．５６", 0, "l1"));
  CHECK(env.numNodeSeeds() == 3);
}

TEST_CASE("numeric unk nodes including interfix are spawned") {
  NumericTestEnv env{"x,l1\nほげ,l2\n"};
  env.analyze("３ほげ１４ぶんの５");
  CHECK(env.contains("３", 0, "l1"));
  CHECK(env.contains("１４ぶんの５", 3, "l1"));
  CHECK(env.contains("４ぶんの５", 4, "l1"));
  CHECK(env.contains("５", 8, "l1"));
  CHECK(env.numNodeSeeds() == 5);
}

TEST_CASE("interfix that is not sandwitched between numbers is not spawned") {
  NumericTestEnv env{"x,l1\nほげ,l2\n"};
  env.analyze("１４ぶんのほげ分の５");
  CHECK(!env.contains("１４ぶんの", 0, "l1"));
  CHECK(!env.contains("分の５", 7, "l1"));
  CHECK(env.numNodeSeeds() == 4);
}

TEST_CASE("interfix accesps suffixes only at the end") {
  // Diff: Juman++ v1.02 accepts ３ミリ分の５ミリ
  NumericTestEnv env{"x,l1\nほげ,l2\n"};
  env.analyze("３分の５ミリ，３ミリ分の５ミリ");
  CHECK(env.contains("３分の５ミリ", 0, "l1"));
  CHECK(!env.contains("３ミリ分の５ミリ", 7, "l1"));
  CHECK(env.numNodeSeeds() == 4);
}

TEST_CASE("interfix are not spawned alone") {
  NumericTestEnv env{"x,l1\nほげ,l2\n"};
  env.analyze("ぶんの");
  CHECK(!env.contains("ぶんの", 0, "l1"));
  CHECK(env.numNodeSeeds() == 0);
}

TEST_CASE("do not make numeric unk nodes ends with period") {
  NumericTestEnv env{"x,l1\nほげ,l2\n"};
  env.analyze("４．");
  CHECK(env.contains("４", 0, "l1"));
  CHECK(!env.contains("４．", 0, "l1"));
  CHECK(env.numNodeSeeds() == 1);
}

TEST_CASE("do not make numeric unk nodes starts with period") {
  NumericTestEnv env{"x,l1\nほげ,l2\n"};
  env.analyze("．４");
  CHECK(env.contains("４", 1, "l1"));
  CHECK(!env.contains("．４", 0, "l1"));
  CHECK(env.numNodeSeeds() == 1);
}

TEST_CASE("make camma separated numeric unk nodes") {
  NumericTestEnv env{"x,l1\nほげ,l2\n"};
  env.analyze("４，１２３，１２３");
  CHECK(env.contains("４，１２３，１２３", 0, "l1"));
  CHECK(env.numNodeSeeds() == 7);
}

TEST_CASE(
    "do not make camma separated numeric unk nodes that do not satisfy camma "
    "separated number") {
  NumericTestEnv env{"x,l1\nほげ,l2\n"};
  env.analyze("４，１２４３");
  CHECK(!env.contains("４，１２４３", 0, "l1"));
  CHECK(env.numNodeSeeds() == 5);
}
