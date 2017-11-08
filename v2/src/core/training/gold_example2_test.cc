//
// Created by Arseny Tolmachev on 2017/03/27.
//

#include "gold_example.h"
#include "training_test_common.h"

namespace {
class GoldExample2Env {
 protected:
  testing::TestEnv env;
  AnalyzerMethods am;

 public:
  GoldExample2Env(StringPiece dic, bool katakanaUnks = false) {
    env.beamSize = 3;
    env.spec([katakanaUnks](core::spec::dsl::ModelSpecBuilder& bldr) {
      auto& a = bldr.field(1, "a").strings().trieIndex();
      auto& d = bldr.field(2, "d").strings();
      auto& b = bldr.field(3, "b").strings().stringStorage(d);
      auto& c = bldr.field(4, "c").strings().stringStorage(d);
      bldr.unigram({a});
      bldr.unigram({d});
      bldr.unigram({c});
      bldr.unigram({a, d});
      bldr.bigram({a}, {a});
      bldr.bigram({d}, {d});
      bldr.bigram({a, d}, {a, d});
      bldr.bigram({a, c}, {a, c});
      bldr.trigram({a}, {a}, {a});
      bldr.train().field(c, 1.0f).field(d, 1.0f).field(b, 0.0f).field(a, 1.0f);

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
    am.initialize(env.analyzer.get());
  }

  const core::spec::AnalysisSpec& spec() const { return env.originalSpec; }

  testing::TestAnalyzer* anaImpl() { return env.analyzer.get(); }

  const core::CoreHolder& core() const { return *env.core; }

  ExampleData firstNode(LatticeNodePtr ptr) { return am.firstNode(ptr); }

  ExampleData top1Node(int idx) { return am.top1Node(idx); }
};
}  // namespace

TEST_CASE("can read simple example with aliased node") {
  StringPiece dic = "X,X,Y,Z\nもも,1,a,A\nも,2,b,B\n";
  StringPiece ex = "A_1_X_もも B_2_Y_も A_1_Z_もも # test\n";
  GoldExample2Env env{dic};
  auto& spec = env.spec();
  auto tspec = spec.training;
  CHECK(tspec.surfaceIdx == 3);
  CHECK(tspec.fields.size() == 4);
  FullyAnnotatedExample exobj;
  FullExampleReader tdr;
  TrainingIo tio;
  REQUIRE_OK(tio.initialize(spec.training, env.core()));
  tdr.setTrainingIo(&tio);
  REQUIRE_OK(tdr.initDoubleCsv(ex));
  auto anaImpl = env.anaImpl();
  REQUIRE_OK(tdr.readFullExample(anaImpl->extraNodesContext(), &exobj));
  CHECK(exobj.numNodes() == 3);
  TrainingExampleAdapter adapter{&tspec, anaImpl};
  REQUIRE_OK(anaImpl->setNewInput(exobj.surface()));
  CHECK(exobj.surface() == "ももももも");
  REQUIRE_OK(anaImpl->prepareNodeSeeds());
  GoldenPath gpath;
  REQUIRE_FALSE(adapter.ensureNodes(exobj, &gpath));
  REQUIRE_OK(anaImpl->buildLattice());
  auto nodes = gpath.nodes();
  CHECK(env.firstNode(nodes[0]) == (ExampleData{"もも", "a", "A"}));
  CHECK(env.firstNode(nodes[1]) == (ExampleData{"も", "b", "B"}));
  CHECK(env.firstNode(nodes[2]) == (ExampleData{"もも", "a", "A"}));
}

TEST_CASE("can read simple example with hidden node") {
  StringPiece dic = "X,X,Y,Z\nもも,1,a,A\nも,2,c,B\nも,2,b,B\n";
  StringPiece ex = "A_1_X_もも B_2_Y_も A_1_Z_もも # test\n";
  GoldExample2Env env{dic};
  auto& spec = env.spec();
  auto tspec = spec.training;
  FullyAnnotatedExample exobj;
  FullExampleReader tdr;
  TrainingIo tio;
  REQUIRE_OK(tio.initialize(spec.training, env.core()));
  tdr.setTrainingIo(&tio);
  REQUIRE_OK(tdr.initDoubleCsv(ex));
  auto anaImpl = env.anaImpl();
  REQUIRE_OK(tdr.readFullExample(anaImpl->extraNodesContext(), &exobj));
  CHECK(exobj.numNodes() == 3);
  TrainingExampleAdapter adapter{&tspec, anaImpl};
  REQUIRE_OK(anaImpl->setNewInput(exobj.surface()));
  CHECK(exobj.surface() == "ももももも");
  REQUIRE_OK(anaImpl->prepareNodeSeeds());
  GoldenPath gpath;
  REQUIRE_FALSE(adapter.ensureNodes(exobj, &gpath));
  REQUIRE_OK(anaImpl->buildLattice());
  auto nodes = gpath.nodes();
  CHECK(env.firstNode(nodes[0]) == (ExampleData{"もも", "a", "A"}));
  CHECK(env.firstNode(nodes[1]) == (ExampleData{"も", "c", "B"}));
  CHECK(env.firstNode(nodes[2]) == (ExampleData{"もも", "a", "A"}));
}