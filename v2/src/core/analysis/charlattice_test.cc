//
// Created by Hajime Morita on 2017/02/23.
//

#include "dictionary_node_creator.h"
#include "normalized_node_creator.h"
#include "core/dic_builder.h"
#include "core/dictionary.h"
#include "core/spec/spec_dsl.h"
#include "testing/test_analyzer.h"
#include "util/string_piece.h"
#include "util/logging.hpp"

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
      specBldr.unk("normalize", 1)
          .normalize() 
          .outputTo({a});
    });
    CHECK(tenv.saveLoad.unkCreators.size() == 1);
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
        WARN("NODE(" << seed.codepointStart << ","<< seed.codepointEnd << "):" << fld1[walker] << "||" << fld2[walker]);
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
    if (!Catch::getResultCapture().getLastResult()->isOk()) {
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

TEST_CASE("charlattice works with youon") {
  CharLatticeTestEnv env{"x,l1\nせが,l2\n"};
  env.analyze("せがぁ");
  CHECK(env.exists("せがぁ", 0));
  CHECK(env.numNodeSeeds() == 2);
}

TEST_CASE("charlattice do not makes a node with irrelevant youon") {
  CharLatticeTestEnv env{"x,l1\nせが,l2\n"};
  env.analyze("せがぃ");
  CHECK(! env.exists("せがぃ", 0));
  CHECK(env.numNodeSeeds() == 1);
}

