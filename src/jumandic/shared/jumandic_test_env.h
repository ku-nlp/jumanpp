//
// Created by Arseny Tolmachev on 2017/05/24.
//

#ifndef JUMANPP_JUMANDIC_TEST_ENV_H
#define JUMANPP_JUMANDIC_TEST_ENV_H

#include <fstream>
#include "core/impl/graphviz_format.h"
#include "core/training/training_env.h"
#include "jumandic/shared/juman_format.h"
#include "jumandic/shared/jumandic_spec.h"
#include "jumandic_spec.h"
#include "testing/test_analyzer.h"
#include "util/lazy.h"

using namespace jumanpp;
namespace {
class JumandicTrainingTestEnv {
 public:
  testing::TestEnv testEnv;
  core::JumanppEnv jppEnv;
  util::Lazy<core::training::TrainingEnv> trainEnv;
  jumandic::output::JumandicFields fields;
  core::training::TrainingArguments trainArgs;

  void globalBeam(i32 left, i32 check, i32 right) {
    trainArgs.globalBeam.minLeftBeam = left;
    trainArgs.globalBeam.maxLeftBeam = 2 * left;
    trainArgs.globalBeam.minRightCheck = check;
    trainArgs.globalBeam.maxRightCheck = 2 * check;
    trainArgs.globalBeam.minRightBeam = right;
    trainArgs.globalBeam.maxRightBeam = 2 * right;
  }

  JumandicTrainingTestEnv(StringPiece dicName, i32 beamSize = 3) {
    trainArgs.trainingConfig.beamSize = beamSize;
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
    trainArgs.batchSize = 100;
  }

  void loadPartialData(StringPiece filename) {
    REQUIRE_OK(trainEnv.value().loadPartialExamples(filename));
  }

  void singleEpochFrom(StringPiece filename) {
    initialize();

    REQUIRE_OK(trainEnv.value().loadInput(filename));
    trainEnv.value().changeGlobalBeam(1);
    REQUIRE_OK(trainEnv.value().trainOneEpoch());
  }

  double trainNepochsFrom(StringPiece filename, i32 number) {
    initialize();
    auto &env = trainEnv.value();
    REQUIRE_OK(env.loadInput(filename));

    for (int i = 0; i < number; ++i) {
      float ratio = static_cast<float>(i) / number;
      env.changeGlobalBeam(ratio);
      REQUIRE_OK(env.loadInput(filename));
      REQUIRE_OK(env.trainOneEpoch());
      if (env.epochLoss() == 0) {
        break;
      }
    }
    return env.epochLoss();
  }

  void initialize() {
    if (!trainEnv.isInitialized()) {
      trainEnv.initialize(trainArgs, &jppEnv);
      REQUIRE_OK(trainEnv.value().initFeatures(nullptr));
      REQUIRE_OK(trainEnv.value().initOther());
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
      REQUIRE_OK(fmt.initialize(train->outputMgr()));
      fmt.reset();
      train->markGold(
          [&](core::analysis::LatticeNodePtr lnp) { fmt.markGold(lnp); });
      auto lat = train->lattice();
      REQUIRE_OK(fmt.render(lat));
      out << fmt.result();
    }
  }
};
}  // namespace

#endif  // JUMANPP_JUMANDIC_TEST_ENV_H
