//
// Created by Arseny Tolmachev on 2017/03/27.
//

#include "gold_example.h"
#include "training_test_common.h"

TEST_CASE("we can create a testing environment") {
  StringPiece dic = "もも,1,1\nも,2,2\n";
  StringPiece ex = "もも,1,1\nも,2,2\nもも,1,1\n";
  GoldExampleEnv env{dic};
  auto& spec = env.spec();
  CHECK(spec.training.surfaceIdx == 0);
  CHECK(spec.training.fields.size() == 2);
  FullyAnnotatedExample exobj;
  FullExampleReader tdr;
  TrainingIo tio;
  REQUIRE_OK(tio.initialize(spec.training, env.core()));
  tdr.setTrainingIo(&tio);
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
  FullExampleReader tdr;
  TrainingIo tio;
  REQUIRE_OK(tio.initialize(spec.training, env.core()));
  tdr.setTrainingIo(&tio);
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
  FullExampleReader tdr;
  TrainingIo tio;
  REQUIRE_OK(tio.initialize(spec.training, env.core()));
  tdr.setTrainingIo(&tio);
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
  FullExampleReader tdr;
  TrainingIo tio;
  REQUIRE_OK(tio.initialize(spec.training, env.core()));
  tdr.setTrainingIo(&tio);
  REQUIRE_OK(tdr.initCsv(ex));
  auto anaImpl = env.anaImpl();
  REQUIRE_OK(tdr.readFullExample(anaImpl->extraNodesContext(), &exobj));
  CHECK(exobj.numNodes() == 3);
  TrainingExampleAdapter adapter{&spec.training, anaImpl};
  REQUIRE_OK(anaImpl->setNewInput(exobj.surface()));
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