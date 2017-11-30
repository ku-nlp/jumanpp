//
// Created by Arseny Tolmachev on 2017/05/24.
//

#include "jumandic/shared/jumandic_test_env.h"
#include "jumandic/shared/lattice_format.h"

TEST_CASE("lattice output works") {
  JumandicTrainingTestEnv env{"jumandic/bug-28-lattice.mdic"};
  env.trainArgs.batchSize = 100;
  env.trainArgs.trainingConfig.featureNumberExponent = 15;
  env.trainNepochsFrom("jumandic/bug-28-lattice.in", 3);
  jumanpp::jumandic::output::LatticeFormat fmt{3};
  auto ana = env.trainEnv.value().makeAnalyzer(5);
  fmt.initialize(ana->output());
  CHECK_OK(ana->analyze("見ぬふりをする"));
  fmt.format(*ana, "");
  CHECK_THAT(fmt.result().str(), Catch::Contains("1;2;3"));
}

TEST_CASE("lattice output works with gbeam", "[gbeam]") {
  JumandicTrainingTestEnv env{"jumandic/bug-28-lattice.mdic"};
  env.globalBeam(3, 1, 3);
  env.trainArgs.batchSize = 100;
  env.trainArgs.trainingConfig.featureNumberExponent = 15;
  env.trainNepochsFrom("jumandic/bug-28-lattice.in", 3);
  // env.dumpTrainers("/tmp/jpp-dbg");
  jumanpp::jumandic::output::LatticeFormat fmt{3};
  auto ana = env.trainEnv.value().makeAnalyzer(5);
  fmt.initialize(ana->output());
  CHECK_OK(ana->analyze("見ぬふりをする"));
  fmt.format(*ana, "");
  CHECK_THAT(fmt.result().str(), Catch::Contains("1;2;3"));
}
