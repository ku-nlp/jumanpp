//
// Created by Arseny Tolmachev on 2017/05/10.
//

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
    trainArgs.batchSize = 2;
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
};

TEST_CASE("jumanpp can correctly read stuff") {
  JumandicTrainingTestEnv env{"jumandic/jumanpp_minimal.mdic"};
  env.initialize();
  REQUIRE_OK(env.trainEnv.value().loadInput("jumandic/train_mini_01.txt"));
  REQUIRE_OK(env.trainEnv.value().readOneBatch());
  REQUIRE_OK(env.trainEnv.value().trainOneBatch());
  REQUIRE_OK(env.trainEnv.value().trainOneBatch());
  CHECK(env.trainEnv.value().batchLoss() == Approx(0.0f));
}

TEST_CASE("jumanpp can learn with minidic") {
  JumandicTrainingTestEnv env{"jumandic/jumanpp_minimal.mdic"};
  env.singleEpochFrom("jumandic/train_mini_01.txt");
  env.singleEpochFrom("jumandic/train_mini_01.txt");
  env.singleEpochFrom("jumandic/train_mini_01.txt");
  env.singleEpochFrom("jumandic/train_mini_01.txt");
  env.singleEpochFrom("jumandic/train_mini_01.txt");
  env.singleEpochFrom("jumandic/train_mini_01.txt");
  env.singleEpochFrom("jumandic/train_mini_01.txt");
  env.singleEpochFrom("jumandic/train_mini_01.txt");
  env.singleEpochFrom("jumandic/train_mini_01.txt");
  env.singleEpochFrom("jumandic/train_mini_01.txt");
  env.singleEpochFrom("jumandic/train_mini_01.txt");
  env.singleEpochFrom("jumandic/train_mini_01.txt");
  env.singleEpochFrom("jumandic/train_mini_01.txt");
}