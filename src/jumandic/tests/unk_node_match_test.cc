//
// Created by Arseny Tolmachev on 2017/11/27.
//

#include "jumandic/shared/jumandic_test_env.h"

using Catch::Matchers::WithinAbs;

TEST_CASE("unk node is correctly selected as a gold one") {
  JumandicTrainingTestEnv env{"jumandic/codegen.mdic"};
  env.trainArgs.batchSize = 20;
  env.initialize();
  auto loss = env.trainNepochsFrom("jumandic/unk_ex.data", 5);
  CHECK_THAT(loss, WithinAbs(0, 1e-3f));
  auto tr = env.trainEnv.value().trainer(0);
  auto realTr = dynamic_cast<core::training::OwningFullTrainer*>(tr);
  CHECK(realTr != nullptr);
  auto& inner = realTr->trainer();
  auto pos = inner.goldenPath().nodes().at(3);
  auto lat = realTr->lattice();
  auto& info =
      lat->boundary(pos.boundary)->starts()->nodeInfo().at(pos.position);
  CHECK(info.numCodepoints() == 6);
}