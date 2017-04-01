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

  float computeTrainingLoss();

  util::ArraySlice<ScoredFeature> featureDiff() const {
    return loss_.featureDiff();
  }
};

}  // namespace training
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_TRAINER_H
