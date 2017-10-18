//
// Created by Arseny Tolmachev on 2017/10/12.
//

#include "jumandic/shared/jumandic_test_env.h"

TEST_CASE("can train with combined examples (boundary only)") {
  JumandicTrainingTestEnv env{"jumandic/jumanpp_minimal.mdic"};
  env.trainArgs.batchSize = 20;
  env.initialize();
  env.loadPartialData("jumandic/partial_01.data");
  env.singleEpochFrom("jumandic/train_mini_01.txt");
  CHECK(env.trainEnv.value().numTrainers() == 7);
  env.trainEnv.value().prepareTrainers();
  CHECK(env.trainEnv.value().numTrainers() == 11);
  for (int i = 0; i < 10; ++i) {
    env.singleEpochFrom("jumandic/train_mini_01.txt");
  }

  // env.dumpTrainers("/tmp/jpp-debug");
  CHECK(env.trainEnv.value().epochLoss() == Approx(0));
}