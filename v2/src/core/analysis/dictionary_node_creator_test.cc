//
// Created by Arseny Tolmachev on 2017/02/23.
//

#include "dictionary_node_creator.h"
#include "core/dic_builder.h"
#include "core/dictionary.h"
#include "core/spec/spec_dsl.h"
#include "testing/standalone_test.h"
#include "testing/test_analyzer.h"
#include "util/string_piece.h"

using namespace jumanpp::core::analysis;
using namespace jumanpp::core::spec;
using namespace jumanpp::core::dic;
using namespace jumanpp::testing;
using namespace jumanpp;

using jumanpp::StringPiece;

class NodeCreatorTestEnv {
  TestEnv tenv;
  StringField fld;

 public:
  NodeCreatorTestEnv(StringPiece csvData) {
    tenv.spec([](dsl::ModelSpecBuilder &specBldr) {
      specBldr.field(1, "a").strings().trieIndex();
    });
    tenv.importDic(csvData);
    REQUIRE_OK(tenv.analyzer->output().stringField("a", &fld));
  }

  void analyze(StringPiece str) {
    CAPTURE(str);
    CHECK_OK(tenv.analyzer->resetForInput(str));
    CHECK_OK(tenv.analyzer->makeNodeSeedsFromDic());
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
    for (auto& seed: seeds) {
      if (seed.codepointStart == start && seed.codepointEnd == end) {
        CHECK(output.locate(seed.entryPtr, &walker));
        while (walker.next()) {
          auto actual = fld[walker];
          if (actual == str) {
            return true;
          }
        }
      }
    }
    return false;
  }
};


TEST_CASE("it is possible to extract some nodes from input string") {
  NodeCreatorTestEnv env{
      "of\na\nan\napple\nabout\nargument\narg\ngum\nmug\nrgu\nfme"};
  env.analyze("argumentofme");
  CHECK(env.exists("arg", 0));
  CHECK(env.exists("gum", 2));
}

class NodeCreatorTestEnv2 {
  TestEnv tenv;
  StringField flda;
  StringField fldb;

public:
  NodeCreatorTestEnv2(StringPiece csvData) {
    tenv.spec([](dsl::ModelSpecBuilder &specBldr) {
      specBldr.field(1, "a").strings().trieIndex();
      specBldr.field(2, "b").strings();
    });
    tenv.importDic(csvData);
    REQUIRE_OK(tenv.analyzer->output().stringField("a", &flda));
    REQUIRE_OK(tenv.analyzer->output().stringField("b", &fldb));
  }

  void analyze(StringPiece str) {
    CAPTURE(str);
    CHECK_OK(tenv.analyzer->resetForInput(str));
    CHECK_OK(tenv.analyzer->makeNodeSeedsFromDic());
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
    for (auto& seed: seeds) {
      if (seed.codepointStart == start && seed.codepointEnd == end) {
        CHECK(output.locate(seed.entryPtr, &walker));
        while (walker.next()) {
          if (flda[walker] == str && fldb[walker] == strb) {
            return true;
          }
        }
      }
    }
    return false;
  }
};

TEST_CASE("extraction works with two different entries") {
  NodeCreatorTestEnv2 env {
      "diag,1\ndump,5\ncor,1\ncor,2\ncors,8"
  };
  env.analyze("zcors");
  CHECK(env.contains("cor", 1, "2"));
  CHECK(env.contains("cor", 1, "1"));
  CHECK_FALSE(env.contains("cors", 1, "7"));
  CHECK(env.contains("cors", 1, "8"));
}
