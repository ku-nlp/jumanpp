//
// Created by Arseny Tolmachev on 2017/03/27.
//

#include "trainer.h"
#include <random>
#include "util/logging.hpp"

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
    addedGoldNodes_ = true;
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

Status Trainer::compute(const analysis::ScorerDef *sconf) {
  loss_.computeGoldScores(sconf);
  JPP_RETURN_IF_ERROR(analyzer->computeScores(sconf));
  JPP_RETURN_IF_ERROR(loss_.resolveTop1());
  JPP_RETURN_IF_ERROR(loss_.computeComparison());
  return Status::Ok();
}

Status TrainerBatch::initialize(const TrainerFullConfig &tfc,
                                const analysis::ScorerDef *sconf,
                                i32 numTrainers) {
  trainers_.clear();
  trainers_.reserve(numTrainers);
  for (int i = 0; i < numTrainers; ++i) {
    auto trainer =
        std::unique_ptr<OwningFullTrainer>(new OwningFullTrainer{tfc});
    JPP_RETURN_IF_ERROR(trainer->initAnalyzer(sconf));
    trainers_.emplace_back(std::move(trainer));
  }
  seed_ = tfc.trainingConfig.randomSeed;
  config_ = &tfc;
  scorerDef_ = sconf;
  return Status::Ok();
}

Status TrainerBatch::readFullBatch(FullExampleReader *rdr) {
  indices_.clear();
  current_ = 0;
  int trIdx = 0;
  for (; trIdx < trainers_.size(); ++trIdx) {
    auto &tr = trainers_[trIdx];
    tr->reset();
    JPP_RETURN_IF_ERROR(tr->readExample(rdr));
    if (rdr->finished()) {
      break;
    }
  }
  current_ = trIdx;

  return Status::Ok();
}

void TrainerBatch::shuffleData(bool usePartial) {
  if (usePartial) {
    totalTrainers_ = activeFullTrainers() + partialTrainerts_.size();
  } else {
    totalTrainers_ = activeFullTrainers();
  }
  indices_.reserve(totalTrainers_);

  for (int i = 0; i < activeFullTrainers(); ++i) {
    indices_.push_back(i);
  }

  if (usePartial) {
    for (int i = 0; i < partialTrainerts_.size(); ++i) {
      indices_.push_back(~i);
    }
  }

  JPP_DCHECK_EQ(indices_.size(), totalTrainers());

  std::minstd_rand rng{seed_ * (static_cast<u32>(numShuffles_) * 31 + 5)};
  std::shuffle(indices_.begin(), indices_.end(), rng);
  numShuffles_ += 1;
}

Status TrainerBatch::readPartialExamples(PartialExampleReader *reader) {
  std::unique_ptr<OwningPartialTrainer> trainer;
  trainer.reset(new OwningPartialTrainer);
  bool eof = true;
  do {
    JPP_RETURN_IF_ERROR(
        reader->readExample(&trainer->trainer_.value().example(), &eof));
    if (eof) {
      break;
    }
    JPP_DCHECK(config_);
    JPP_DCHECK(scorerDef_);
    JPP_RETURN_IF_ERROR(trainer->initialize(*config_, *scorerDef_));
    partialTrainerts_.emplace_back(trainer.release());
  } while (!eof);
  return Status::Ok();
}

ITrainer *TrainerBatch::trainer(i32 idx) const {
  JPP_DCHECK_EQ(totalTrainers(), indices_.size());
  JPP_DCHECK_IN(idx, 0, totalTrainers());
  auto idx2 = indices_[idx];
  if (idx2 >= 0) {
    JPP_DCHECK_IN(idx2, 0, activeFullTrainers());
    return trainers_[idx2].get();
  } else {
    auto idx3 = ~idx2;
    JPP_DCHECK_IN(idx3, 0, partialTrainerts_.size());
    return partialTrainerts_[idx3].get();
  }
}

}  // namespace training
}  // namespace core
}  // namespace jumanpp