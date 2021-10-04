//
// Created by Arseny Tolmachev on 2017/10/12.
//

#include "jumandic/shared/jumandic_test_env.h"

using Catch::Matchers::WithinAbs;

TEST_CASE("can train with combined examples (boundary only)") {
  JumandicTrainingTestEnv env{"jumandic/jumanpp_minimal.mdic"};
  env.trainArgs.batchSize = 20;
  env.initialize();
  env.loadPartialData("jumandic/partial_01.data");
  env.singleEpochFrom("jumandic/train_mini_01.txt");
  CHECK(env.trainEnv.value().numTrainers() == 7);
  env.trainEnv.value().prepareTrainers();
  CHECK(env.trainEnv.value().numTrainers() == 11);
  auto loss = env.trainNepochsFrom("jumandic/train_mini_01.txt", 10);

  // env.dumpTrainers("/tmp/jpp-debug");
  CHECK_THAT(loss, WithinAbs(0, 1e-3));
}

TEST_CASE("can train with part and gbeam", "[gbeam]") {
  JumandicTrainingTestEnv env{"jumandic/jumanpp_minimal.mdic"};
  env.trainArgs.batchSize = 20;
  env.globalBeam(3, 1, 3);
  env.initialize();
  env.loadPartialData("jumandic/partial_01.data");
  env.singleEpochFrom("jumandic/train_mini_01.txt");
  CHECK(env.trainEnv.value().numTrainers() == 7);
  env.trainEnv.value().prepareTrainers();
  CHECK(env.trainEnv.value().numTrainers() == 11);
  auto loss = env.trainNepochsFrom("jumandic/train_mini_01.txt", 10);

  // env.dumpTrainers("/tmp/jpp-debug");
  CHECK_THAT(loss, WithinAbs(0, 1e-3));
}