//
// Created by Arseny Tolmachev on 2017/11/15.
//

#include "cg_2_spec.h"
#include "cgtest02.h"
#include "core/analysis/perceptron.h"
#include "testing/test_analyzer.h"

using namespace jumanpp::testing;
using namespace jumanpp::core::spec::dsl;
using namespace jumanpp;

TEST_CASE("generated pattern features are equal to provided") {
  TestEnv env;
  env.spec([](ModelSpecBuilder& msb) {
    jumanpp::codegentest::CgTwoSpecFactory::fillSpec(msb);
  });
  env.importDic("a,b,c\na,d,e\na,g,h\na,z,z\na,y,y\na,f,f\n");
  ScorerDef sdef;
  sdef.scoreWeights.push_back(1);
  float weights[] = {1.0, 1.1, 1.2, 1.3};
  core::analysis::HashedFeaturePerceptron hfp{weights};
  sdef.feature = &hfp;
  REQUIRE(env.analyzer->initScorers(sdef));
  env.analyzer->resetForInput("a");
  REQUIRE(env.analyzer->prepareNodeSeeds());
  REQUIRE(env.analyzer->buildLattice());
  REQUIRE(env.analyzer->bootstrapAnalysis());
  REQUIRE(env.analyzer->computeScores(&sdef));

  JumanppEnv jenv1;
  env.loadEnv(&jenv1);
  jumanpp_generated::Test02 features;
  REQUIRE(jenv1.initFeatures(&features));
  ScoringConfig scoreConf{env.beamSize, 1};
  AnalyzerImpl impl2{jenv1.coreHolder(), scoreConf, env.aconf};
  REQUIRE(impl2.initScorers(sdef));
  impl2.resetForInput("a");
  REQUIRE(impl2.prepareNodeSeeds());
  REQUIRE(impl2.buildLattice());
  REQUIRE(impl2.bootstrapAnalysis());
  REQUIRE(impl2.computeScores(&sdef));

  auto p1 =
      env.analyzer->lattice()->boundary(2)->starts()->patternFeatureData();
  auto p2 = impl2.lattice()->boundary(2)->starts()->patternFeatureData();
  REQUIRE(p1.size() == p2.size());
  REQUIRE(p1.numRows() == p2.numRows());
  for (int row = 0; row < p1.numRows(); ++row) {
    CAPTURE(row);
    for (int idx = 0; idx < p1.rowSize(); ++idx) {
      CAPTURE(idx);
      CHECK(p1.row(row).at(idx) == p2.row(row).at(idx));
    }
  }
}