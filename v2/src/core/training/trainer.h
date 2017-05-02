//
// Created by Arseny Tolmachev on 2017/03/27.
//

#ifndef JUMANPP_TRAINER_H
#define JUMANPP_TRAINER_H

#include "core/analysis/perceptron.h"
#include "gold_example.h"
#include "loss.h"
#include "training_io.h"
#include "training_types.h"

namespace jumanpp {
namespace core {
namespace training {

class Trainer {
  analysis::AnalyzerImpl* analyzer;
  FullyAnnotatedExample example_;
  TrainingExampleAdapter adapter_;
  LossCalculator loss_;
  TrainingConfig config_;
  float currentLoss_;

 public:
  Trainer(analysis::AnalyzerImpl* analyzer, const spec::TrainingSpec* spec,
          const TrainingConfig& trconf)
      : analyzer{analyzer},
        adapter_{spec, analyzer},
        loss_{analyzer, spec},
        config_{trconf} {}

  void reset() {
    analyzer->reset();
    loss_.goldPath().reset();
    adapter_.reset();
  }

  FullyAnnotatedExample& example() { return example_; }

  Status prepare();

  Status compute(analysis::ScoreConfig* sconf);

  void computeTrainingLoss();

  float lossValue() const { return currentLoss_; }

  util::ArraySlice<ScoredFeature> featureDiff() const {
    return loss_.featureDiff();
  }
};

struct TrainerFullConfig {
  const analysis::AnalyzerConfig& analyzerConfig;
  const CoreHolder& core;
  const spec::TrainingSpec& trainingSpec;
  const TrainingConfig& trainingConfig;
};

class OwningTrainer {
  analysis::AnalyzerImpl analyzer_;
  Trainer trainer_;
  bool wasPrepared = false;

 public:
  OwningTrainer(const TrainerFullConfig& conf)
      : analyzer_{&conf.core, conf.analyzerConfig},
        trainer_{&analyzer_, &conf.trainingSpec, conf.trainingConfig} {}

  void reset() {
    wasPrepared = false;
    trainer_.reset();
  }

  Status readExample(TrainingDataReader* rdr) {
    return rdr->readFullExample(analyzer_.extraNodesContext(),
                                &trainer_.example());
  }

  Status prepare() {
    if (wasPrepared) return Status::Ok();
    wasPrepared = true;
    return trainer_.prepare();
  }

  Status compute(analysis::ScoreConfig* sconf) {
    JPP_RETURN_IF_ERROR(trainer_.compute(sconf));
    trainer_.computeTrainingLoss();
    return Status::Ok();
  }

  float loss() const { return trainer_.lossValue(); }

  util::ArraySlice<ScoredFeature> featureDiff() const {
    return trainer_.featureDiff();
  }
};

class BatchedTrainer {
  TrainerFullConfig config_;
  std::vector<std::unique_ptr<OwningTrainer>> trainers_;
  i32 current_;

 public:
  BatchedTrainer(const TrainerFullConfig& tfc, i32 numTrainers) : config_{tfc} {
    trainers_.reserve(numTrainers);
    for (int i = 0; i < numTrainers; ++i) {
      trainers_.emplace_back(new OwningTrainer{config_});
    }
  }

  Status readBatch(TrainingDataReader* rdr) {
    current_ = 0;
    int trIdx = 0;
    for (; trIdx < trainers_.size() && !rdr->finished(); ++trIdx) {
      auto& tr = trainers_[trIdx];
      tr->reset();
      JPP_RETURN_IF_ERROR(tr->readExample(rdr));
    }
    current_ = trIdx;
    return Status::Ok();
  }
};

}  // namespace training
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_TRAINER_H
