//
// Created by Arseny Tolmachev on 2017/03/27.
//

#include "gold_example.h"
#include <ostream>
#include "testing/test_analyzer.h"

using namespace jumanpp;
using namespace jumanpp::core::training;
using namespace jumanpp::core::analysis;

struct ExampleData {
  StringPiece a;
  StringPiece b;
  StringPiece c;

  bool operator==(const ExampleData& o) const noexcept {
    return a == o.a && b == o.b && c == o.c;
  }

  friend std::ostream& operator<<(std::ostream& os, const ExampleData& data) {
    os << "{a:" << data.a << " b:" << data.b << " c:" << data.c << "}";
    return os;
  }
};

class GoldExampleEnv {
  testing::TestEnv env;
  StringField fa;
  StringField fb;
  StringField fc;

 public:
  GoldExampleEnv(StringPiece dic) {
    env.spec([](core::spec::dsl::ModelSpecBuilder& bldr) {
      auto& a = bldr.field(1, "a").strings().trieIndex();
      auto& b = bldr.field(2, "b").strings();
      bldr.field(3, "c").strings();
      bldr.unigram({a, b});
      bldr.bigram({a}, {a});
      bldr.bigram({b}, {b});
      bldr.bigram({a, b}, {a, b});
      bldr.trigram({a}, {a}, {a});
      bldr.train().field(a, 1.0f).field(b, 1.0f);
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
};

TEST_CASE("we can create a testing environment") {
  StringPiece dic = "もも,1,1\nも,2,2\n";
  StringPiece ex = "もも,1,1\nも,2,2\nもも,1,1\n";
  GoldExampleEnv env{dic};
  auto& spec = env.spec();
  CHECK(spec.training.surfaceIdx == 0);
  CHECK(spec.training.fields.size() == 2);
  FullyAnnotatedExample exobj;
  TrainingDataReader tdr;
  REQUIRE_OK(tdr.initialize(spec.training, env.core()));
  REQUIRE_OK(tdr.initCsv(ex));
  auto anaImpl = env.anaImpl();
  REQUIRE_OK(tdr.readFullExample(anaImpl->extraNodesContext(), &exobj));
  CHECK(exobj.numNodes() == 3);
  TrainingExampleAdapter adapter{&spec.training, anaImpl};
  anaImpl->resetForInput(exobj.surface());
  REQUIRE_OK(anaImpl->prepareNodeSeeds());
  GoldenPath gpath;
  adapter.ensureNodes(exobj, &gpath);
  REQUIRE_OK(anaImpl->buildLattice());
  REQUIRE_OK(adapter.repointPathPointers(exobj, &gpath));
  auto nodes = gpath.nodes();
  CHECK(env.firstNode(nodes[0]) == (ExampleData{"もも", "1", "1"}));
  CHECK(env.firstNode(nodes[1]) == (ExampleData{"も", "2", "2"}));
  CHECK(env.firstNode(nodes[2]) == (ExampleData{"もも", "1", "1"}));
}

TEST_CASE("we can create a testing environment with merging nodes") {
  StringPiece dic = "もも,1,1\nもも,1,10\nも,2,2\nも,2,11\nも,3,3\nも,3,12\n";
  StringPiece ex = "もも,1,1\nも,3,12\nもも,1,10\n";
  GoldExampleEnv env{dic};
  auto& spec = env.spec();
  CHECK(spec.training.surfaceIdx == 0);
  CHECK(spec.training.fields.size() == 2);
  FullyAnnotatedExample exobj;
  TrainingDataReader tdr;
  REQUIRE_OK(tdr.initialize(spec.training, env.core()));
  REQUIRE_OK(tdr.initCsv(ex));
  auto anaImpl = env.anaImpl();
  REQUIRE_OK(tdr.readFullExample(anaImpl->extraNodesContext(), &exobj));
  CHECK(exobj.numNodes() == 3);
  TrainingExampleAdapter adapter{&spec.training, anaImpl};
  anaImpl->resetForInput(exobj.surface());
  REQUIRE_OK(anaImpl->prepareNodeSeeds());
  GoldenPath gpath;
  CHECK_FALSE(adapter.ensureNodes(exobj, &gpath));
  REQUIRE_OK(anaImpl->buildLattice());
  REQUIRE_OK(adapter.repointPathPointers(exobj, &gpath));
  auto nodes = gpath.nodes();
  CHECK(env.firstNode(nodes[0]) == (ExampleData{"もも", "1", "1"}));
  CHECK(env.firstNode(nodes[1]) == (ExampleData{"も", "3", "3"}));
  CHECK(env.firstNode(nodes[2]) == (ExampleData{"もも", "1", "1"}));
}

TEST_CASE("we can create a testing environment with unk nodes") {
  StringPiece dic = "もも,1,1\nも,2,2\nが,3,3\n";
  StringPiece ex = "もも,1,1\nも,2,2\nもも,3,3\n";
  GoldExampleEnv env{dic};
  auto& spec = env.spec();
  CHECK(spec.training.surfaceIdx == 0);
  CHECK(spec.training.fields.size() == 2);
  FullyAnnotatedExample exobj;
  TrainingDataReader tdr;
  REQUIRE_OK(tdr.initialize(spec.training, env.core()));
  REQUIRE_OK(tdr.initCsv(ex));
  auto anaImpl = env.anaImpl();
  REQUIRE_OK(tdr.readFullExample(anaImpl->extraNodesContext(), &exobj));
  CHECK(exobj.numNodes() == 3);
  TrainingExampleAdapter adapter{&spec.training, anaImpl};
  anaImpl->resetForInput(exobj.surface());
  REQUIRE_OK(anaImpl->prepareNodeSeeds());
  GoldenPath gpath;
  CHECK(adapter.ensureNodes(exobj, &gpath));
  env.anaImpl()->latticeBldr()->sortSeeds();
  CHECK_OK(env.anaImpl()->latticeBldr()->prepare());
  REQUIRE_OK(anaImpl->buildLattice());
  REQUIRE_OK(adapter.repointPathPointers(exobj, &gpath));
  auto nodes = gpath.nodes();
  CHECK(env.firstNode(nodes[0]) == (ExampleData{"もも", "1", "1"}));
  CHECK(env.firstNode(nodes[1]) == (ExampleData{"も", "2", "2"}));
  CHECK(env.firstNode(nodes[2]) == (ExampleData{"もも", "3", ""}));
}

TEST_CASE(
    "we can create a testing environment with unk nodes and unk surface") {
  StringPiece dic = "もも,1,1\nも,2,2\nが,3,3\n";
  StringPiece ex = "もも,1,1\nも,2,2\nうめ,3,3\n";
  GoldExampleEnv env{dic};
  auto& spec = env.spec();
  CHECK(spec.training.surfaceIdx == 0);
  CHECK(spec.training.fields.size() == 2);
  FullyAnnotatedExample exobj;
  TrainingDataReader tdr;
  REQUIRE_OK(tdr.initialize(spec.training, env.core()));
  REQUIRE_OK(tdr.initCsv(ex));
  auto anaImpl = env.anaImpl();
  REQUIRE_OK(tdr.readFullExample(anaImpl->extraNodesContext(), &exobj));
  CHECK(exobj.numNodes() == 3);
  TrainingExampleAdapter adapter{&spec.training, anaImpl};
  anaImpl->setNewInput(exobj.surface());
  REQUIRE_FALSE(anaImpl->prepareNodeSeeds());
  CHECK_OK(env.anaImpl()->latticeBldr()->prepare());
  GoldenPath gpath;
  CHECK(adapter.ensureNodes(exobj, &gpath));
  env.anaImpl()->latticeBldr()->sortSeeds();
  CHECK_OK(env.anaImpl()->latticeBldr()->prepare());
  CHECK(env.anaImpl()->latticeBuilder().checkConnectability());
  REQUIRE_OK(anaImpl->buildLattice());
  REQUIRE_OK(adapter.repointPathPointers(exobj, &gpath));
  auto nodes = gpath.nodes();
  CHECK(env.firstNode(nodes[0]) == (ExampleData{"もも", "1", "1"}));
  CHECK(env.firstNode(nodes[1]) == (ExampleData{"も", "2", "2"}));
  CHECK(env.firstNode(nodes[2]) == (ExampleData{"うめ", "3", ""}));
}