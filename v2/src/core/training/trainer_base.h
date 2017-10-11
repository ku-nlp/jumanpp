//
// Created by Arseny Tolmachev on 2017/10/07.
//

#ifndef JUMANPP_TRAINER_BASE_H
#define JUMANPP_TRAINER_BASE_H

#include "core/analysis/score_api.h"
#include "core/training/training_io.h"
#include "core/training/training_types.h"

namespace jumanpp {
namespace core {
namespace training {

struct ScoredFeature {
  u32 feature;
  float score;
};

struct TrainerFullConfig {
  const analysis::AnalyzerConfig& analyzerConfig;
  const CoreHolder& core;
  const spec::TrainingSpec& trainingSpec;
  const TrainingConfig& trainingConfig;
};

class ITrainer {
 public:
  virtual ~ITrainer() = default;
  virtual Status prepare() = 0;
  virtual Status compute(const analysis::ScorerDef* sconf) = 0;
  virtual float loss() const = 0;
  virtual util::ArraySlice<ScoredFeature> featureDiff() const = 0;
  virtual ExampleInfo exampleInfo() const = 0;

  virtual const analysis::OutputManager& outputMgr() const = 0;
  virtual void markGold(
      std::function<void(analysis::LatticeNodePtr)> callback) const = 0;
  virtual analysis::Lattice* lattice() const = 0;
};

}  // namespace training
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_TRAINER_BASE_H
