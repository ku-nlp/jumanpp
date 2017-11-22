//
// Created by Hajime Morita on 2017/07/21.
//

#include "core/dic/dic_builder.h"
#include "core/dic/dictionary.h"
#include "core/spec/spec_dsl.h"
#include "dictionary_node_creator.h"
#include "normalized_node_creator.h"
#include "testing/test_analyzer.h"
#include "util/logging.hpp"
#include "util/string_piece.h"

using namespace jumanpp::core::analysis;
using namespace jumanpp::core::spec;
using namespace jumanpp::core::dic;
using namespace jumanpp::testing;
using namespace jumanpp;

using jumanpp::StringPiece;

namespace {
class CharLatticeTestEnv {
  TestEnv tenv;
  StringField fld1, fld2;

  DictionaryEntries dic{tenv.dic.entries()};

 public:
  CharLatticeTestEnv(StringPiece csvData) {
    tenv.spec([](dsl::ModelSpecBuilder& specBldr) {
      auto& a = specBldr.field(1, "f1").strings().trieIndex();
      specBldr.field(2, "f2").strings();
      specBldr.unk("normalize", 1).normalize().outputTo({a});
    });
    CHECK(tenv.originalSpec.unkCreators.size() == 1);
    tenv.importDic(csvData);
    REQUIRE_OK(tenv.analyzer->output().stringField("f1", &fld1));
    REQUIRE_OK(tenv.analyzer->output().stringField("f2", &fld2));
  }

  size_t numNodeSeeds() const {
    return tenv.analyzer->latticeBuilder().seeds().size();
  }

  void printAll() {
    auto& output = tenv.analyzer->output();
    auto walker = output.nodeWalker();
    auto seeds = tenv.analyzer->latticeBuilder().seeds();

    for (auto& seed : seeds) {
      CHECK(output.locate(seed.entryPtr, &walker));
      while (walker.next()) {
        LOG_TRACE() << "NODE(" << seed.codepointStart << ","
                    << seed.codepointEnd << "):" << fld1[walker] << "||"
                    << fld2[walker];
      }
    }
  }

  void analyze(StringPiece str) {
    CAPTURE(str);
    CHECK_OK(tenv.analyzer->resetForInput(str));
    CHECK_OK(tenv.analyzer->makeNodeSeedsFromDic());
    CHECK_OK(tenv.analyzer->makeUnkNodes1());
  }

  bool exists(StringPiece str, i32 start) {
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
          auto actual = fld1[walker];
          if (actual == str) {
            return true;
          }
        }
      }
    }
    return false;
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

  ~CharLatticeTestEnv() {
    if (!Catch::getResultCapture().lastAssertionPassed()) {
      printAll();
    }
  }
};
}  // namespace

TEST_CASE("charlattice runs without failure") {
  CharLatticeTestEnv env{"x,l1\nほげ,l2\n"};
  env.analyze("ほげほげ");
  CHECK(env.contains("ほげ", 0, "l2"));
  CHECK(env.exists("ほげ", 2));
  CHECK(env.numNodeSeeds() == 2);
}

TEST_CASE("charlattice works with choon insertion") {
  CharLatticeTestEnv env{"x,l1\nほげ,l2\n"};
  env.analyze("ほげほーげ");
  CHECK(env.contains("ほげ", 0, "l2"));
  CHECK(env.exists("ほーげ", 2));
  CHECK(env.numNodeSeeds() == 2);
}

TEST_CASE("charlattice works with choon postfix") {
  CharLatticeTestEnv env{"x,l1\nほげ,l2\n"};
  env.analyze("ほげーーほげ");
  CHECK(env.exists("ほげー", 0));
  CHECK(env.exists("ほげーー", 0));
  CHECK(env.numNodeSeeds() == 4);
}

TEST_CASE("charlattice works with choon insertion and postfix") {
  CharLatticeTestEnv env{"x,l1\nせが,l2\n"};
  env.analyze("せーがーー");
  CHECK(env.exists("せーがー", 0));
  CHECK(env.exists("せーが", 0));
  CHECK(env.numNodeSeeds() == 3);
}

TEST_CASE("Kanji accepts choon only if next character is Hiragana") {
  CharLatticeTestEnv env{"x,l2\n走る,l2\n蹴,l2\n"};
  env.analyze("走ーる蹴ー");
  CHECK(env.exists("走ーる", 0));
  CHECK(env.exists("蹴", 3));
  CHECK(env.exists("蹴ー", 3));
  CHECK(env.numNodeSeeds() == 3);
}

TEST_CASE("charlattice works with youon") {
  CharLatticeTestEnv env{"x,l1\nせが,l2\n"};
  env.analyze("せがぁぁ");
  CHECK(env.exists("せがぁ", 0));
  CHECK(env.exists("せがぁぁ", 0));
  CHECK(env.numNodeSeeds() == 3);
}

TEST_CASE("charlattice works with youon and hatsuon") {
  CharLatticeTestEnv env{"x,l1\nせが,l2\n"};
  env.analyze("せがぁぁっ");
  CHECK(env.exists("せが", 0));
  CHECK(env.exists("せがぁ", 0));
  CHECK(env.exists("せがぁぁ", 0));
  CHECK(env.exists("せがぁぁっ", 0));
  CHECK(env.numNodeSeeds() == 4);
}

TEST_CASE("charlattice works with youon and choon") {
  CharLatticeTestEnv env{"x,l1\nせが,l2\n"};
  env.analyze("せがぁぁーー");
  CHECK(env.exists("せが", 0));
  CHECK(env.exists("せがぁぁー", 0));
  CHECK(env.exists("せがぁぁーー", 0));
  CHECK(env.numNodeSeeds() == 5);
}

TEST_CASE("charlattice substitutes choon to vowel") {
  CharLatticeTestEnv env{"x,l1\nせがあ,l2\n"};
  env.analyze("せがー");
  CHECK(env.exists("せがー", 0));
  CHECK(env.numNodeSeeds() == 1);
}

TEST_CASE("charlattice substitutes small vowel to vowel") {
  CharLatticeTestEnv env{"x,l1\nせがあ,l2\n"};
  env.analyze("せがぁ");
  CHECK(env.exists("せがぁ", 0));
  CHECK(env.numNodeSeeds() == 1);
}

TEST_CASE("charlattice do not makes a node with irrelevant youon") {
  CharLatticeTestEnv env{"x,l1\nせが,l2\n"};
  env.analyze("せがぃ");
  CHECK(!env.exists("せがぃ", 0));
  CHECK(env.numNodeSeeds() == 1);
}

TEST_CASE("charlattice makes a node with last hatsuon") {
  CharLatticeTestEnv env{"x,l1\nせが,l2\n"};
  env.analyze("せがっ");
  CHECK(env.exists("せが", 0));
  CHECK(env.exists("せがっ", 0));
  CHECK(env.numNodeSeeds() == 2);
}

TEST_CASE("charlattice makes a node with hatsuon inside hiragana") {
  CharLatticeTestEnv env{"x,l1\nせが,l2\n"};
  env.analyze("せっが");
  CHECK(env.exists("せっが", 0));
  CHECK(env.numNodeSeeds() == 1);
}

TEST_CASE("charlattice makes a node with hatsuon inside katakana") {
  CharLatticeTestEnv env{"x,l1\nセガ,l2\n"};
  env.analyze("セッガ");
  CHECK(env.exists("セッガ", 0));
  CHECK(env.numNodeSeeds() == 1);
}

TEST_CASE("charlattice doesn't make a node with mixed chars") {
  CharLatticeTestEnv env{"x,l1\nせガ,l2\n"};
  env.analyze("せッガ");
  CHECK(env.numNodeSeeds() == 0);
}

TEST_CASE("charlattice doesn't make a node with kanji") {
  CharLatticeTestEnv env{"x,l1\n引張る,l2\n"};
  env.analyze("引っ張る");
  CHECK(env.numNodeSeeds() == 0);
}

TEST_CASE("charlattice makes a node including hatsuon inside") {
  CharLatticeTestEnv env{"x,l1\nせが,l2\n"};
  env.analyze("せーっが");
  CHECK(env.exists("せーっが", 0));
  CHECK(env.numNodeSeeds() == 1);
}

TEST_CASE("charlattice makes a node with choon and hatsuon") {
  CharLatticeTestEnv env{"x,l1\nせが,l2\n"};
  env.analyze("せがーっ");
  CHECK(env.exists("せが", 0));
  CHECK(env.exists("せがー", 0));
  CHECK(env.exists("せがーっ", 0));
  CHECK(env.numNodeSeeds() == 3);
}

TEST_CASE("charlattice finds prolonged e-row") {
  CharLatticeTestEnv env{"x,l1\nてめえ,l2\n"};
  env.analyze("てめー");
  CHECK(env.exists("てめー", 0));
  CHECK(env.numNodeSeeds() == 1);
}

TEST_CASE("handles multiple prolongings correctly (#7)") {
  CharLatticeTestEnv env{"x,l1\nツリー,l2\n"};
  env.analyze("ツリーーー");
  CHECK(env.exists("ツリーーー", 0));
  CHECK(env.exists("ツリーー", 0));
  CHECK(env.exists("ツリー", 0));
  CHECK(env.numNodeSeeds() == 3);
}

TEST_CASE("handles multiple prolongings with variants correctly") {
  CharLatticeTestEnv env{"x,l1\nつれい,l2\nつれえ,l3\n"};
  env.analyze("つれーーー");
  CHECK(env.contains("つれー", 0, "l2"));
  CHECK(env.contains("つれー", 0, "l3"));
  CHECK(env.contains("つれーー", 0, "l2"));
  CHECK(env.contains("つれーー", 0, "l3"));
  CHECK(env.contains("つれーーー", 0, "l2"));
  CHECK(env.contains("つれーーー", 0, "l3"));
  CHECK(env.numNodeSeeds() == 6);
}

TEST_CASE("don't crash on very long prolong") {
  CharLatticeTestEnv env{"x,l1\nあ,l2\n"};
  env.analyze(
      "あーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーー"
      "ーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーー"
      "ーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーー"
      "ーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーー"
      "ーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーー"
      "ーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーー");
  CHECK(env.numNodeSeeds() == 199);
}
