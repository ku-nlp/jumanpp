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
  return Status::Ok();
}

float Trainer::computeTrainingLoss() {
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
      JPP_DCHECK(false);
      break;
    }
  }
  auto loss = loss_.computeLoss(usedSteps);
  loss_.computeFeatureDiff(config_.numHashedFeatures - 1);
  return loss;
}

Status Trainer::compute(analysis::ScoreConfig *sconf) {
  JPP_RETURN_IF_ERROR(analyzer->computeScores(sconf));
  JPP_RETURN_IF_ERROR(loss_.resolveTop1());
  JPP_RETURN_IF_ERROR(loss_.computeComparison());
  return Status::Ok();
}

}  // training
}  // core
}  // jumanpp