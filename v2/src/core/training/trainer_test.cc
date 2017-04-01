//
// Created by Arseny Tolmachev on 2017/03/31.
//

#include "trainer.h"
#include "training_test_common.h"
#include "scw.h"

using namespace jumanpp::core::training;

class TrainerEnv : public GoldExampleEnv {
public:
  static TrainingConfig testConf() {
    TrainingConfig tc;
    tc.numHashedFeatures = 4096;
    return tc;
  }

  core::training::TrainingDataReader rdr;
  Trainer trainer;
  TrainerEnv(StringPiece s) : GoldExampleEnv(s), trainer{anaImpl(), &env.saveLoad.training, testConf()} {
    rdr.initialize(env.saveLoad.training, core());
  }

  void parseMrph(StringPiece data) {
    REQUIRE_OK(rdr.initDoubleCsv(data));
    REQUIRE_OK(rdr.readFullExample(anaImpl()->extraNodesContext(), &trainer.example()));
  }
};

TEST_CASE("trainer can compute score for a simple sentence") {
  StringPiece dic = "もも,N,0\nも,PRT,1\n";
  StringPiece ex = "もも_N_0 も_PRT_1 もも_N_0\n";
  TrainerEnv env {dic};
  env.parseMrph(ex);
  SoftConfidenceWeighted scw {TrainerEnv::testConf()};
  CHECK(env.trainer.example().numNodes() == 3);
  CHECK(env.trainer.prepare());
  CHECK(env.trainer.compute(scw.scoreConfig()));
  auto loss = env.trainer.computeTrainingLoss();
  CHECK(loss > 0);
}