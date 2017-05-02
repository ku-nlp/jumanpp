//
// Created by Arseny Tolmachev on 2017/03/31.
//

#ifndef JUMANPP_TRAINING_TEST_COMMON_H
#define JUMANPP_TRAINING_TEST_COMMON_H

#include <ostream>
#include "testing/test_analyzer.h"

using namespace jumanpp;
using namespace jumanpp::core::training;
using namespace jumanpp::core::analysis;

struct ExampleData {
  StringPiece a;
  StringPiece b;
  StringPiece c;

  ExampleData(StringPiece a, StringPiece b, StringPiece c) : a{a}, b{b}, c{c} {}

  bool operator==(const ExampleData& o) const noexcept {
    return a == o.a && b == o.b && c == o.c;
  }

  friend std::ostream& operator<<(std::ostream& os, const ExampleData& data) {
    os << "{a:" << data.a << " b:" << data.b << " c:" << data.c << "}";
    return os;
  }
};

class GoldExampleEnv {
 protected:
  testing::TestEnv env;
  StringField fa;
  StringField fb;
  StringField fc;
  AnalysisPath path;

 public:
  GoldExampleEnv(StringPiece dic, bool katakanaUnks = false) {
    env.beamSize = 2;
    env.spec([katakanaUnks](core::spec::dsl::ModelSpecBuilder& bldr) {
      auto& a = bldr.field(1, "a").strings().trieIndex();
      auto& b = bldr.field(2, "b").strings();
      bldr.field(3, "c").strings();
      bldr.unigram({a, b});
      bldr.bigram({a}, {a});
      bldr.bigram({b}, {b});
      bldr.bigram({a, b}, {a, b});
      bldr.trigram({a}, {a}, {a});
      bldr.train().field(a, 1.0f).field(b, 1.0f);

      if (katakanaUnks) {
        bldr.unk("katakana", 1)
            .chunking(chars::CharacterClass::KATAKANA)
            .outputTo({a});
      }
    });
    env.importDic(dic);
    ScoreConfig scoreConfig{};
    REQUIRE_OK(env.analyzer->initScorers(scoreConfig));
    auto omgr = env.analyzer->output();
    REQUIRE_OK(omgr.stringField("a", &fa));
    REQUIRE_OK(omgr.stringField("b", &fb));
    REQUIRE_OK(omgr.stringField("c", &fc));
  }

  const core::spec::AnalysisSpec& spec() const { return env.saveLoad; }

  testing::TestAnalyzer* anaImpl() { return env.analyzer.get(); }

  const core::CoreHolder& core() const { return *env.core; }

  ExampleData firstNode(LatticeNodePtr ptr) {
    auto omgr = env.analyzer->output();
    auto w = omgr.nodeWalker();
    REQUIRE(omgr.locate(ptr, &w));
    REQUIRE(w.next());
    return {fa[w], fb[w], fc[w]};
  }

  ExampleData top1Node(int idx) {
    REQUIRE_OK(path.fillIn(env.analyzer->lattice()));
    path.moveToBoundary(idx);
    REQUIRE(path.totalNodesInChunk() == 1);
    ConnectionPtr ptr;
    REQUIRE(path.nextNode(&ptr));
    return firstNode({ptr.boundary, ptr.right});
  }
};

#endif  // JUMANPP_TRAINING_TEST_COMMON_H
