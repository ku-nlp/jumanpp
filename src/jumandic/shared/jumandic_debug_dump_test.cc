//
// Created by Arseny Tolmachev on 2017/12/08.
//

#include "core/proto/lattice_dump_output.h"
#include "jumandic/shared/jumandic_test_env.h"

using namespace jumanpp;

TEST_CASE("debug dump works with jumandic and full beam") {
  JumandicTrainingTestEnv env{"jumandic/codegen.mdic"};
  core::analysis::Analyzer an;
  core::analysis::AnalyzerConfig analyzerConfig;
  analyzerConfig.storeAllPatterns = true;
  analyzerConfig.globalBeamSize = 0;
  env.initialize();
  core::ScoringConfig sconf;
  sconf.numScorers = 1;
  sconf.beamSize = 5;
  REQUIRE(an.initialize(env.jppEnv.coreHolder(), analyzerConfig, sconf,
                        env.trainEnv.value().scorerDef()));
  an.impl()->setStoreAllPatterns(true);
  REQUIRE(an.analyze("５５１年もガラフケマペが兵をつの〜ってたな！"));
  core::output::LatticeDumpOutput output;
  auto& weights = env.trainEnv.value().scorerDef()->feature->weights();
  REQUIRE(output.initialize(an.impl(), &weights));
  REQUIRE(output.format(an, "test"));
}