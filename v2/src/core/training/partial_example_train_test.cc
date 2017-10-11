//
// Created by Arseny Tolmachev on 2017/10/11.
//

#include <fstream>
#include "core/impl/graphviz_format.h"
#include "core/training/scw.h"
#include "partial_example.h"
#include "training_test_common.h"

namespace {
class TrainerEnv : public GoldExampleEnv {
 public:
  static TrainingConfig testConf() {
    TrainingConfig tc;
    tc.featureNumberExponent = 12;
    return tc;
  }
  core::training::TrainingIo tio;
  core::training::PartialExampleReader rdr;
  core::training::PartialTrainer trainer;
  TrainerEnv(StringPiece s, bool kataUnks = false)
      : GoldExampleEnv(s, kataUnks), trainer{anaImpl()} {
    REQUIRE_OK(tio.initialize(env.saveLoad.training, core()));
    REQUIRE_OK(rdr.initialize(&tio));
  }

  void parseMrph(StringPiece data) {
    rdr.setData(data);
    bool eof = false;
    REQUIRE_OK(rdr.readExample(&trainer.example(), &eof));
    REQUIRE_FALSE(eof);
  }

  std::unique_ptr<testing::TestAnalyzer> newAnalyzer(const ScorerDef* sconf) {
    std::unique_ptr<testing::TestAnalyzer> ptr{new testing::TestAnalyzer{
        env.core.get(),
        {this->env.beamSize, (i32)sconf->scoreWeights.size()},
        env.aconf}};
    ptr->initScorers(*sconf);
    return ptr;
  }

  void dumpTrainers(StringPiece baseDir, int cnt = 0) {
    auto bldr = core::format::GraphVizBuilder{};
    bldr.row({"a", "b", "c"});
    core::format::GraphVizFormat fmt;
    REQUIRE_OK(bldr.build(&fmt, env.beamSize));

    char buffer[256];

    std::snprintf(buffer, 255, "%s/dump_%d.dot", baseDir.char_begin(), cnt);
    std::ofstream out{buffer};
    auto pana = env.analyzer.get();
    REQUIRE_OK(fmt.initialize(pana->output()));
    fmt.reset();
    trainer.markGold(
        [&](const core::analysis::LatticeNodePtr ptr) { fmt.markGold(ptr); },
        pana->lattice());
    auto lat = pana->lattice();
    REQUIRE_OK(fmt.render(lat));
    out << fmt.result();
  }
};
}  // namespace

TEST_CASE("can train a simple example") {
  TrainerEnv env{"UNK,N,5\nもも,N,0\nも,PRT,1\nモ,PRT,2"};
  env.parseMrph("もも\nも\nもも\n\n");
  CHECK(env.trainer.prepare());
  SoftConfidenceWeighted scw{TrainerEnv::testConf()};
  CHECK(env.trainer.compute(scw.scorers()));
  CHECK(env.trainer.loss() > 0);
  double total = 0;
  for (auto& e : env.trainer.featureDiff()) {
    total += e.score;
  }
  CHECK(total == Approx(0.0));
}

TEST_CASE("can train a simple example with tags") {
  TrainerEnv env{"UNK,N,5\nもも,N,0\nも,PRT,1\nモ,PRT,2"};
  env.parseMrph("\tもも\n\tもも\n\tもも\n\n");
  CHECK(env.trainer.prepare());
  SoftConfidenceWeighted scw{TrainerEnv::testConf()};
  CHECK(env.trainer.compute(scw.scorers()));
  CHECK(env.trainer.loss() > 0);
  double total = 0;
  for (auto& e : env.trainer.featureDiff()) {
    total += e.score;
  }
  CHECK(total == Approx(0.0));
}