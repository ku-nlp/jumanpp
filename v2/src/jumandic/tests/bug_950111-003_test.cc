//
// Created by Arseny Tolmachev on 2017/05/24.
//

#include "jumandic/shared/jumandic_test_env.h"

TEST_CASE("example id S-ID:950110271-003 works") {
  JumandicTrainingTestEnv env{"jumandic/bug950111-003.mdic"};
  env.initialize();
  env.singleEpochFrom("jumandic/bug950111-003.in");
  env.singleEpochFrom("jumandic/bug950111-003.in");
  env.singleEpochFrom("jumandic/bug950111-003.in");
  env.singleEpochFrom("jumandic/bug950111-003.in");
  CHECK(env.trainEnv.value().batchLoss() == Approx(0.0f));
}
