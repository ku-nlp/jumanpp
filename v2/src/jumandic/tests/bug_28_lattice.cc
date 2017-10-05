//
// Created by Arseny Tolmachev on 2017/05/24.
//

#include "jumandic/shared/jumandic_test_env.h"
#include "jumandic/shared/lattice_format.h"

TEST_CASE("lattice output works") {
  JumandicTrainingTestEnv env{"jumandic/bug-28-lattice.mdic"};
  env.trainArgs.batchSize = 100;
  env.trainArgs.trainingConfig.featureNumberExponent = 15;
  env.initialize();
  env.singleEpochFrom("jumandic/bug-28-lattice.in");
  env.singleEpochFrom("jumandic/bug-28-lattice.in");
  env.singleEpochFrom("jumandic/bug-28-lattice.in");
  env.singleEpochFrom("jumandic/bug-28-lattice.in");
  jumanpp::jumandic::output::LatticeFormat fmt{3};
  auto ana = env.trainEnv.value().makeAnalyzer(5);
  fmt.initialize(ana->output());
  CHECK_OK(ana->analyze("見ぬふりをする"));
  fmt.format(*ana, "");
  CHECK_THAT(fmt.result().str(), Catch::Contains("1;2;3"));
}
