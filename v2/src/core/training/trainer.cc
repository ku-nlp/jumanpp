//
// Created by Arseny Tolmachev on 2017/03/27.
//

#include "trainer.h"

namespace jumanpp {
namespace core {
namespace training {

Status Trainer::prepare() {
  auto latBldr = analyzer->latticeBldr();

  JPP_RETURN_IF_ERROR(analyzer->setNewInput(example_.surface()));
  Status seedStatus = analyzer->prepareNodeSeeds();

  if (!seedStatus) {
    // lattice was discontinous
    // but we need latticeinfo for building gold nodes
    latBldr->sortSeeds();
    JPP_RETURN_IF_ERROR(latBldr->prepare());
  }

  if (adapter_.ensureNodes(example_, &loss_.goldPath())) {
    // ok, we have created at least one more node
    // that should have fixed any connectability errors
    latBldr->sortSeeds();
    if (!latBldr->checkConnectability()) {
      return Status::InvalidState()
             << "could not build lattice for gold example";
    }
    JPP_RETURN_IF_ERROR(latBldr->prepare());
  } else {
    // prepareSeedNodes function could have exited with error
    // we haven't created new nodes here, so check that status
    if (!seedStatus) {
      return seedStatus;
    }
  }

  JPP_RETURN_IF_ERROR(analyzer->buildLattice());
  JPP_RETURN_IF_ERROR(
      adapter_.repointPathPointers(example_, &loss_.goldPath()));
  analyzer->bootstrapAnalysis();
  JPP_RETURN_IF_ERROR(loss_.resolveGold());
  return Status::Ok();
}

void Trainer::computeTrainingLoss() {
  i32 usedSteps = -1;
  switch (config_.mode) {
    case TrainingMode::Full: {
      usedSteps = loss_.fullSize();
      break;
    }
    case TrainingMode::FalloffBeam: {
      usedSteps = loss_.fallOffBeam();
      break;
    }
    case TrainingMode::MaxViolation: {
      usedSteps = loss_.maxViolation();
      break;
    }
  }
  currentLoss_ = loss_.computeLoss(usedSteps);
  loss_.computeFeatureDiff(config_.numFeatures() - 1);
}

Status Trainer::compute(const analysis::ScoreConfig *sconf) {
  loss_.computeGoldScores(sconf);
  JPP_RETURN_IF_ERROR(analyzer->computeScores(sconf));
  JPP_RETURN_IF_ERROR(loss_.resolveTop1());
  JPP_RETURN_IF_ERROR(loss_.computeComparison());
  return Status::Ok();
}

}  // namespace training
}  // namespace core
}  // namespace jumanpp