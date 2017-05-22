//
// Created by Arseny Tolmachev on 2017/05/10.
//

#include <fstream>
#include "core/impl/graphviz_format.h"
#include "core/training/training_env.h"
#include "jumandic/shared/juman_format.h"
#include "jumandic/shared/jumandic_spec.h"
#include "testing/test_analyzer.h"
#include "util/lazy.h"

using namespace jumanpp;

class JumandicTrainingTestEnv {
 public:
  testing::TestEnv testEnv;
  core::JumanppEnv jppEnv;
  util::Lazy<core::training::TrainingEnv> trainEnv;
  jumandic::output::JumandicFields fields;
  core::training::TrainingArguments trainArgs;

  JumandicTrainingTestEnv(StringPiece dicName) {
    trainArgs.trainingConfig.beamSize = 3;
    trainArgs.batchSize = 5;
    trainArgs.numThreads = 1;
    testEnv.spec([](core::spec::dsl::ModelSpecBuilder &sb) {
      jumandic::SpecFactory::fillSpec(sb);
    });
    util::MappedFile inputFile;
    REQUIRE_OK(inputFile.open(dicName, util::MMapType::ReadOnly));
    util::MappedFileFragment frag;
    REQUIRE_OK(inputFile.map(&frag, 0, inputFile.size()));
    testEnv.importDic(frag.asStringPiece(), dicName);
    testEnv.loadEnv(&jppEnv);
    REQUIRE_OK(fields.initialize(testEnv.analyzer->output()));
    trainArgs.trainingConfig.featureNumberExponent = 14;  // 16k
  }

  void singleEpochFrom(StringPiece filename) {
    initialize();

    REQUIRE_OK(trainEnv.value().loadInput(filename));
    REQUIRE_OK(trainEnv.value().trainOneEpoch());
  }

  void initialize() {
    if (!trainEnv.isInitialized()) {
      trainEnv.initialize(trainArgs, &jppEnv);
      REQUIRE_OK(trainEnv.value().initialize());
      REQUIRE_OK(trainEnv.value().initFeatures(nullptr));
    }
  }

  void dumpTrainers(StringPiece baseDir) {
    auto bldr = core::format::GraphVizBuilder{};
    bldr.row({"reading", "baseform"});
    bldr.row({"canonic", "surface"});
    bldr.row({"pos", "subpos"});
    bldr.row({"conjtype", "conjform"});
    core::format::GraphVizFormat fmt;
    REQUIRE_OK(bldr.build(&fmt, trainArgs.trainingConfig.beamSize));

    char buffer[256];
    auto &te = trainEnv.value();
    for (int i = 0; i < te.numTrainers(); ++i) {
      std::snprintf(buffer, 255, "%s/%04d.dot", baseDir.char_begin(), i);
      std::ofstream out{buffer};
      auto train = te.trainer(i);
      auto pana = train->anaImpl();
      REQUIRE_OK(fmt.initialize(pana->output()));
      fmt.reset();
      auto gnodes = train->trainer()->goldenPath().nodes();
      for (auto &node : gnodes) {
        fmt.markGold(node);
      }
      auto lat = pana->lattice();
      REQUIRE_OK(fmt.render(lat));
      out << fmt.result();
    }
  }
};

TEST_CASE("jumanpp can correctly read stuff") {
  JumandicTrainingTestEnv env{"jumandic/jumanpp_minimal.mdic"};
  env.initialize();
  REQUIRE_OK(env.trainEnv.value().loadInput("jumandic/train_mini_01.txt"));
  REQUIRE_OK(env.trainEnv.value().readOneBatch());
  REQUIRE_OK(env.trainEnv.value().trainOneBatch());
  // env.dumpTrainers("/tmp/dots/1");
  REQUIRE_OK(env.trainEnv.value().loadInput("jumandic/train_mini_01.txt"));
  REQUIRE_OK(env.trainEnv.value().readOneBatch());
  REQUIRE_OK(env.trainEnv.value().trainOneBatch());
  // env.dumpTrainers("/tmp/dots/2");
  REQUIRE_OK(env.trainEnv.value().trainOneBatch());
  CHECK(env.trainEnv.value().batchLoss() == Approx(0.0f));
}

TEST_CASE("jumanpp can learn with minidic") {
  JumandicTrainingTestEnv env{"jumandic/jumanpp_minimal.mdic"};
  env.singleEpochFrom("jumandic/train_mini_01.txt");
  env.singleEpochFrom("jumandic/train_mini_01.txt");
  env.singleEpochFrom("jumandic/train_mini_01.txt");
  env.singleEpochFrom("jumandic/train_mini_01.txt");
}