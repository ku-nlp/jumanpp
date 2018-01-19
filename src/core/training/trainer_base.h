//
// Created by Arseny Tolmachev on 2017/10/07.
//

#ifndef JUMANPP_TRAINER_BASE_H
#define JUMANPP_TRAINER_BASE_H

#include "core/analysis/analyzer.h"
#include "core/analysis/score_api.h"
#include "core/input/training_io.h"
#include "core/training/training_types.h"

namespace jumanpp {
namespace core {
namespace training {

using core::input::ExampleInfo;

struct ScoredFeature {
  u32 feature;
  float score;
};

struct TrainerFullConfig {
  const analysis::AnalyzerConfig* analyzerConfig;
  const CoreHolder* core;
  const spec::TrainingSpec* trainingSpec;
  const TrainingConfig* trainingConfig;
};

struct GlobalBeamTrainConfig {
  i32 leftBeam = -1;
  i32 rightBeam = -1;
  i32 rightCheck = -1;

  bool isEnabled() const { return leftBeam > 0; }
};

std::ostream& operator<<(std::ostream& o, const GlobalBeamTrainConfig& cfg);

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
  virtual void setGlobalBeam(const GlobalBeamTrainConfig& cfg) = 0;
};

}  // namespace training
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_TRAINER_BASE_H
