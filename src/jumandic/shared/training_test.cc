//
// Created by Arseny Tolmachev on 2017/05/10.
//

#include "jumandic_test_env.h"

using Catch::Matchers::WithinAbs;

TEST_CASE("jumanpp can correctly read stuff") {
  JumandicTrainingTestEnv env{"jumandic/jumanpp_minimal.mdic"};
  env.initialize();
  REQUIRE_OK(env.trainEnv.value().loadInput("jumandic/train_mini_01.txt"));
  REQUIRE_OK(env.trainEnv.value().readOneBatch());
  env.trainEnv.value().prepareTrainers();
  REQUIRE_OK(env.trainEnv.value().trainOneBatch(0));
  // env.dumpTrainers("/tmp/dots/1");
  REQUIRE_OK(env.trainEnv.value().loadInput("jumandic/train_mini_01.txt"));
  REQUIRE_OK(env.trainEnv.value().readOneBatch());
  env.trainEnv.value().prepareTrainers();
  REQUIRE_OK(env.trainEnv.value().trainOneBatch(0));
  // env.dumpTrainers("/tmp/dots/2");
  REQUIRE_OK(env.trainEnv.value().trainOneBatch(1));
  REQUIRE_OK(env.trainEnv.value().trainOneBatch(2));
  CHECK_THAT(env.trainEnv.value().batchLoss(), WithinAbs(0.0f, 1e-4f));
}

TEST_CASE("jumanpp can learn with minidic") {
  JumandicTrainingTestEnv env{"jumandic/jumanpp_minimal.mdic"};
  env.singleEpochFrom("jumandic/train_mini_01.txt");
  env.singleEpochFrom("jumandic/train_mini_01.txt");
  env.singleEpochFrom("jumandic/train_mini_01.txt");
  env.singleEpochFrom("jumandic/train_mini_01.txt");
}