//
// Created by Arseny Tolmachev on 2017/10/11.
//

#include <fstream>
#include "core/impl/graphviz_format.h"
#include "core/training/scw.h"
#include "partial_example.h"
#include "training_test_common.h"
#include "util/logging.hpp"

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
  explicit TrainerEnv(StringPiece s, bool kataUnks = false)
      : GoldExampleEnv(s, kataUnks), trainer{anaImpl(), (1 << 12) - 1} {
    REQUIRE_OK(tio.initialize(env.originalSpec.training, core()));
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

TEST_CASE("correctly computes matching nodes") {
  TrainerEnv env{"UNK,N,5\nもも,N,0\nも,PRT,1\nモ,PRT,2"};
  env.parseMrph("\tもも\nも\n\tもも\n\tもも\nも\n\n");
  CHECK(env.trainer.prepare());
  auto ex = env.trainer.example();
  auto l = env.anaImpl()->lattice();
  auto checkNode = [&](u16 bnd, u16 pos, StringPiece surface, bool result) {
    CAPTURE(bnd);
    CAPTURE(pos);
    CHECK(ex.doesNodeMatch(l, bnd, pos) == result);
    CHECK(env.firstNode(LatticeNodePtr{bnd, pos}).a == surface);
  };
  checkNode(2, 0, "も", false);
  checkNode(2, 1, "もも", true);
  checkNode(3, 0, "も", false);
  checkNode(3, 1, "もも", false);
  checkNode(4, 0, "も", true);
  checkNode(4, 1, "もも", false);
  checkNode(5, 0, "も", false);
  checkNode(5, 1, "もも", true);
  checkNode(6, 0, "も", false);
  checkNode(6, 1, "もも", false);
  checkNode(7, 0, "も", false);
  checkNode(7, 1, "もも", true);
  checkNode(8, 0, "も", false);
  checkNode(8, 1, "もも", false);
  checkNode(9, 0, "も", true);
}

TEST_CASE("can compute loss/features from a simple example bnds/words") {
  TrainerEnv env{"UNK,N,5\nもも,N,0\nも,PRT,1\nも,PRA,2"};
  env.parseMrph("\tもも\nも\n\tもも\n\tもも\n\tも\tb:PRT\n\tも\tb:PRA\n\n");
  CHECK(env.trainer.prepare());
  SoftConfidenceWeighted scw{TrainerEnv::testConf()};
  CHECK(env.trainer.compute(scw.scorers()));
  CHECK(env.trainer.loss() > 0);
  // env.dumpTrainers("/tmp/jpp-dbg", 0);
  double total = 0;
  for (auto& e : env.trainer.featureDiff()) {
    total += e.score;
  }
  CHECK(total == Approx(0.0));
}

TEST_CASE("can compute loss/features from a simple example with tags") {
  TrainerEnv env{"UNK,N,5\nもも,N,0\nも,PRT,1\nも,ZAP,3\nモ,PRT,2"};
  env.parseMrph("\tもも\n\tも\tb:ZAP\n\tもも\n\tもも\nも\n\n");
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

TEST_CASE("can decrease loss/features from a simple example with tags") {
  TrainerEnv env{"UNK,N,5\nもも,N,0\nも,PRT,1\nモ,PRT,2"};
  env.parseMrph("\tもも\nも\n\tもも\n\tもも\nモ\n\n");
  CHECK(env.trainer.prepare());
  SoftConfidenceWeighted scw{TrainerEnv::testConf()};
  CHECK(env.trainer.compute(scw.scorers()));
  CHECK(env.trainer.loss() > 0);
  for (int iter = 0; iter < 30; ++iter) {
    // LOG_DEBUG() << "LOSS: " << env.trainer.loss();
    scw.update(env.trainer.loss(), env.trainer.featureDiff());
    CHECK(env.trainer.compute(scw.scorers()));
    // env.dumpTrainers("/tmp/jpp-dbg", iter);
    if (env.trainer.loss() == 0) {
      break;
    }
  }
  auto sconf = scw.scorers();
  env.trainer.compute(sconf);
  CHECK(env.trainer.loss() == Approx(0));
}