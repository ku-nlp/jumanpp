//
// Created by Arseny Tolmachev on 2017/05/24.
//

#include "jumandic/shared/jumandic_test_env.h"

TEST_CASE("example id S-ID:950110271-003 works") {
  JumandicTrainingTestEnv env{"jumandic/bug950111-003.mdic"};
  env.initialize();
  CHECK(env.trainNepochsFrom("jumandic/bug950111-003.in", 10) == Approx(0));
}

TEST_CASE("example id S-ID:950110271-003 works with gbeam", "[gbeam]") {
  JumandicTrainingTestEnv env{"jumandic/bug950111-003.mdic"};
  env.initialize();
  CHECK(env.trainNepochsFrom("jumandic/bug950111-003.in", 10) == Approx(0));
}