//
// Created by Arseny Tolmachev on 2017/03/27.
//

#ifndef JUMANPP_TRAINER_H
#define JUMANPP_TRAINER_H

#include "core/analysis/perceptron.h"
#include "core/input/partial_example_io.h"
#include "core/training/gold_example.h"
#include "core/training/loss.h"
#include "core/training/partial_trainer.h"
#include "core/training/training_types.h"

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
  bool addedGoldNodes_;

 public:
  Trainer(analysis::AnalyzerImpl* analyzer, const spec::TrainingSpec* spec,
          const TrainingConfig& trconf)
      : analyzer{analyzer},
        adapter_{analyzer},
        loss_{analyzer, spec},
        config_{trconf} {}

  void reset() {
    example_.reset();
    resetState();
  }

  void resetState() {
    analyzer->reset();
    loss_.goldPath().reset();
    adapter_.reset();
    addedGoldNodes_ = false;
  }

  FullyAnnotatedExample& example() { return example_; }
  const FullyAnnotatedExample& example() const { return example_; }

  Status prepare();

  Status compute(const analysis::ScorerDef* sconf);

  void computeTrainingLoss();

  float lossValue() const { return currentLoss_; }

  util::ArraySlice<ScoredFeature> featureDiff() const {
    return loss_.featureDiff();
  }

  GoldenPath& goldenPath() { return loss_.goldPath(); }
  const GoldenPath& goldenPath() const { return loss_.goldPath(); }

  bool wasGoldAdded() const { return addedGoldNodes_; }
};

class OwningFullTrainer final : public ITrainer {
  analysis::AnalyzerImpl analyzer_;
  Trainer trainer_;
  bool wasPrepared = false;

 public:
  explicit OwningFullTrainer(const TrainerFullConfig& conf)
      : analyzer_{conf.core, ScoringConfig{conf.trainingConfig->beamSize, 1},
                  *conf.analyzerConfig},
        trainer_{&analyzer_, conf.trainingSpec, *conf.trainingConfig} {}

  void reset() {
    wasPrepared = false;
    trainer_.reset();
  }

  Status initAnalyzer(const analysis::ScorerDef* sconf) {
    return analyzer_.initScorers(*sconf);
  }

  Status readExample(FullExampleReader* rdr) {
    Status s = rdr->readFullExample(&trainer_.example());
    trainer_.example().setInfo(rdr->filename(), rdr->lineNumber());
    return s;
  }

  Status prepare() override {
    if (wasPrepared) return Status::Ok();
    wasPrepared = true;
    return trainer_.prepare();
  }

  Status compute(const analysis::ScorerDef* sconf) override {
    JPP_RETURN_IF_ERROR(trainer_.compute(sconf));
    trainer_.computeTrainingLoss();
    return Status::Ok();
  }

  float loss() const override { return trainer_.lossValue(); }

  ExampleInfo exampleInfo() const override {
    return trainer_.example().exampleInfo();
  }

  util::ArraySlice<ScoredFeature> featureDiff() const override {
    return trainer_.featureDiff();
  }

  const analysis::OutputManager& outputMgr() const override {
    return analyzer_.output();
  }

  void markGold(
      std::function<void(analysis::LatticeNodePtr)> callback) const override {
    for (auto& ex : trainer_.goldenPath().nodes()) {
      callback(ex);
    }
  }

  analysis::Lattice* lattice() const override {
    return const_cast<analysis::AnalyzerImpl&>(analyzer_).lattice();
  }

  const Trainer& trainer() const { return trainer_; }

  void setGlobalBeam(const GlobalBeamTrainConfig& cfg) override;
};

class TrainerBatch {
  TrainerFullConfig config_;
  const analysis::ScorerDef* scorerDef_;
  std::vector<std::unique_ptr<OwningFullTrainer>> trainers_;
  std::vector<std::unique_ptr<OwningPartialTrainer>> partialTrainerts_;
  std::vector<i32> indices_;
  i32 current_;
  i32 totalTrainers_;
  u32 seed_ = 0xdeadbeef;
  i32 numShuffles_ = 0;

 public:
  Status initialize(const TrainerFullConfig& tfc,
                    const analysis::ScorerDef* sconf, i32 numTrainers);

  Status readFullBatch(FullExampleReader* rdr);
  Status readPartialExamples(input::PartialExampleReader* reader);

  void cleanParital() { partialTrainerts_.clear(); }

  void shuffleData(bool usePartial);

  ITrainer* trainer(i32 idx) const;

  i32 activeFullTrainers() const { return current_; }
  i32 totalTrainers() const {
    JPP_DCHECK_EQ(totalTrainers_, indices_.size());
    return totalTrainers_;
  }
};

}  // namespace training
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_TRAINER_H
