//
// Created by Arseny Tolmachev on 2017/03/31.
//

#include "trainer.h"
#include <fstream>
#include "core/impl/graphviz_format.h"
#include "scw.h"
#include "training_test_common.h"

using namespace jumanpp::core::training;

namespace {
class TrainerEnv : public GoldExampleEnv {
 public:
  static TrainingConfig testConf() {
    TrainingConfig tc;
    tc.featureNumberExponent = 12;
    return tc;
  }
  core::training::TrainFieldsIndex tio;
  core::training::FullExampleReader rdr;
  Trainer trainer;
  TrainerEnv(StringPiece s, bool kataUnks = false)
      : GoldExampleEnv(s, kataUnks),
        trainer{anaImpl(), &env.originalSpec.training, testConf()} {
    REQUIRE_OK(tio.initialize(core()));
    rdr.setTrainingIo(&tio);
  }

  void parseMrph(StringPiece data) {
    REQUIRE_OK(rdr.initDoubleCsv(data));
    REQUIRE_OK(rdr.readFullExample(&trainer.example()));
  }

  std::unique_ptr<testing::TestAnalyzer> newAnalyzer(const ScorerDef* sconf) {
    std::unique_ptr<testing::TestAnalyzer> ptr{new testing::TestAnalyzer{
        env.core.get(),
        {this->env.beamSize, (i32)sconf->scoreWeights.size()},
        env.aconf}};
    ptr->initScorers(*sconf);
    return ptr;
  }

  void dumpTrainers(StringPiece baseDir, int cnt = 0) {
    auto bldr = core::format::GraphVizBuilder{};
    bldr.row({"a", "b", "c"});
    core::format::GraphVizFormat fmt;
    REQUIRE_OK(bldr.build(&fmt, env.beamSize));

    char buffer[256];

    std::snprintf(buffer, 255, "%s/dump_%d.dot", baseDir.char_begin(), cnt);
    std::ofstream out{buffer};
    auto pana = env.analyzer.get();
    REQUIRE_OK(fmt.initialize(pana->output()));
    fmt.reset();
    auto gnodes = trainer.goldenPath().nodes();
    for (auto& node : gnodes) {
      fmt.markGold(node);
    }
    auto lat = pana->lattice();
    REQUIRE_OK(fmt.render(lat));
    out << fmt.result();
  }
};
}  // namespace

TEST_CASE("trainer can compute score for a simple sentence") {
  StringPiece dic = "もも,N,0\nも,PRT,1\n";
  StringPiece ex = "もも_N_0 もも_N_0 も_PRT_1 もも_N_0\n";
  TrainerEnv env{dic};
  env.parseMrph(ex);
  SoftConfidenceWeighted scw{TrainerEnv::testConf()};
  CHECK(env.trainer.example().numNodes() == 4);
  CHECK(env.trainer.prepare());
  CHECK(env.trainer.compute(scw.scorers()));
  env.trainer.computeTrainingLoss();
  // env.dumpTrainers("/tmp/dots", 0);
  CHECK(env.trainer.lossValue() > 0);
}

TEST_CASE(
    "trainer can compute score for a simple sentence and update weights") {
  StringPiece dic = "もも,N,0\nも,PRT,1\n";
  StringPiece ex = "もも_N_0 も_PRT_1 もも_N_0\n";
  TrainerEnv env{dic};
  env.parseMrph(ex);
  SoftConfidenceWeighted scw{TrainerEnv::testConf()};
  CHECK(env.trainer.example().numNodes() == 3);
  CHECK(env.trainer.prepare());
  auto mem1 = env.anaImpl()->usedMemory();
  CHECK(env.trainer.compute(scw.scorers()));
  env.trainer.computeTrainingLoss();
  auto mem2 = env.anaImpl()->usedMemory();
  CHECK(env.trainer.lossValue() > 0);
  for (int i = 0; i < 10; ++i) {
    scw.update(env.trainer.lossValue(), env.trainer.featureDiff());
    CHECK(env.trainer.compute(scw.scorers()));
    env.trainer.computeTrainingLoss();
    auto mem3 = env.anaImpl()->usedMemory();
    CHECK(mem1 == mem2);
    CHECK(mem1 == mem3);
  }

  CHECK(env.trainer.lossValue() == 0);
}

TEST_CASE("trainer can parse an example with comment") {
  StringPiece dic = "もも,N,0\nも,PRT,1\n";
  StringPiece ex = "もも_N_0 も_PRT_1 もも_N_0 # help me\n";
  TrainerEnv env{dic};
  env.parseMrph(ex);
  CHECK(env.trainer.example().comment() == "help me");
}

TEST_CASE("trainer can compute score for sentence with full unks") {
  StringPiece dic = "UNK,N,10\nもも,N,0\nも,PRT,1\nモ,PRT,2";
  StringPiece ex = "モモ_N_10 も_PRT_1 もも_N_0\n";
  TrainerEnv env{dic, true};  // use unks
  env.parseMrph(ex);
  SoftConfidenceWeighted scw{TrainerEnv::testConf()};
  CHECK(env.trainer.example().numNodes() == 3);
  CHECK(env.trainer.prepare());
  CHECK(env.trainer.compute(scw.scorers()));
  env.trainer.computeTrainingLoss();
  CHECK(env.trainer.lossValue() > 0);
  scw.update(env.trainer.lossValue(), env.trainer.featureDiff());
  CHECK(env.trainer.compute(scw.scorers()));
  env.trainer.computeTrainingLoss();
  CHECK(env.trainer.lossValue() == 0);
}

TEST_CASE("trainer can compute score for sentence with part unks") {
  StringPiece dic = "UNK,N,5\nもも,N,0\nも,PRT,1\nモ,PRT,2";
  StringPiece ex = "モモ_N_10 も_PRT_1 もも_N_0\n";
  TrainerEnv env{dic, true};  // use unks
  env.parseMrph(ex);
  SoftConfidenceWeighted scw{TrainerEnv::testConf()};
  CHECK(env.trainer.example().numNodes() == 3);
  CHECK(env.trainer.prepare());
  CHECK(env.trainer.compute(scw.scorers()));
  env.trainer.computeTrainingLoss();
  CHECK(env.trainer.lossValue() > 0);
  scw.update(env.trainer.lossValue(), env.trainer.featureDiff());
  CHECK(env.trainer.compute(scw.scorers()));
  env.trainer.computeTrainingLoss();
  CHECK(env.trainer.lossValue() == 0);
  CHECK(env.top1Node(0) == ExampleData("モモ", "N", "5"));
}

TEST_CASE("trainer can compute score for sentence with other POS unks") {
  StringPiece dic = "UNK,N,5\nもも,N,0\nも,PRT,1\nモ,WTF,2\n寝る,V,3";
  StringPiece ex =
      "モモ_V_10 も_PRT_1 モモ_N_5 も_PRT_1 モモ_V_10 も_PRT_1 もも_N_0 "
      "モモ_N_10 も_PRT_1 もも_N_0\n";
  TrainerEnv env{dic, true};  // use unks
  env.parseMrph(ex);
  SoftConfidenceWeighted scw{TrainerEnv::testConf()};
  CHECK(env.trainer.example().numNodes() == 10);
  CHECK(env.trainer.prepare());
  CHECK(env.trainer.compute(scw.scorers()));
  env.trainer.computeTrainingLoss();
  CHECK(env.trainer.lossValue() >= 0);
  for (int i = 0; i < 10; ++i) {
    scw.update(env.trainer.lossValue(), env.trainer.featureDiff());
    CHECK(env.trainer.compute(scw.scorers()));
    env.trainer.computeTrainingLoss();
  }

  CHECK(env.trainer.lossValue() == 0);
  // env.dumpTrainers("/tmp/jpp", 10);

  auto ana2 = env.newAnalyzer(scw.scorers());
  REQUIRE_OK(ana2->fullAnalyze("モモももも", scw.scorers()));
  AnalyzerMethods am{ana2.get()};
  CHECK(am.top1Node(0) == ExampleData("モモ", "N", "5"));
  CHECK(am.top1Node(1) == ExampleData("も", "PRT", "1"));
  CHECK(am.top1Node(2) == ExampleData("もも", "N", "0"));
}