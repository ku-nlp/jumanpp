//
// Created by Arseny Tolmachev on 2017/03/31.
//

#ifndef JUMANPP_TRAINING_TEST_COMMON_H
#define JUMANPP_TRAINING_TEST_COMMON_H

#include <array>
#include <ostream>
#include "core/analysis/analysis_result.h"
#include "testing/test_analyzer.h"

namespace jumanpp {
namespace core {
namespace training {}
}  // namespace core
}  // namespace jumanpp

using namespace jumanpp;
using namespace jumanpp::core::training;
using namespace jumanpp::core::analysis;
using namespace jumanpp::testing;

class GoldExampleEnv {
 protected:
  TestEnv env;
  AnalyzerMethods am;

 public:
  GoldExampleEnv(StringPiece dic, bool katakanaUnks = false) {
    env.beamSize = 3;
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
    ScorerDef scorers{};
    scorers.scoreWeights.push_back(1.0f);
    REQUIRE_OK(env.analyzer->initScorers(scorers));
    REQUIRE(am.initialize(env.analyzer.get()));
  }

  const core::spec::AnalysisSpec& spec() const { return env.originalSpec; }

  testing::TestAnalyzer* anaImpl() { return env.analyzer.get(); }

  const core::CoreHolder& core() const { return *env.core; }

  ExampleData firstNode(LatticeNodePtr ptr) { return am.firstNode(ptr); }

  ExampleData top1Node(int idx) { return am.top1Node(idx); }
};

#endif  // JUMANPP_TRAINING_TEST_COMMON_H
