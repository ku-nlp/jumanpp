//
// Created by Arseny Tolmachev on 2017/05/24.
//

#include "jumandic/shared/jumandic_test_env.h"

using Catch::Matchers::WithinAbs;

TEST_CASE("example id S-ID:950110271-003 works") {
  JumandicTrainingTestEnv env{"jumandic/bug950111-003.mdic"};
  env.initialize();
  CHECK_THAT(env.trainNepochsFrom("jumandic/bug950111-003.in", 10), WithinAbs(0, 1e-3));
}

TEST_CASE("example id S-ID:950110271-003 works with gbeam", "[gbeam]") {
  JumandicTrainingTestEnv env{"jumandic/bug950111-003.mdic"};
  env.initialize();
  CHECK_THAT(env.trainNepochsFrom("jumandic/bug950111-003.in", 10), WithinAbs(0, 1e-3));
}