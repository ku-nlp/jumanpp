//
// Created by Arseny Tolmachev on 2017/10/07.
//

#ifndef JUMANPP_PARTIAL_TRAINER_H
#define JUMANPP_PARTIAL_TRAINER_H

#include <string>
#include <vector>
#include "core/analysis/analysis_result.h"
#include "core/analysis/analyzer_impl.h"
#include "core/training/partial_example.h"
#include "core/training/trainer_base.h"
#include "training_io.h"
#include "util/characters.h"
#include "util/common.hpp"
#include "util/csv_reader.h"
#include "util/mmap.h"
#include "util/types.hpp"

namespace jumanpp {
namespace core {
namespace training {

class PartialTrainer {
  PartialExample example_;
  std::vector<ScoredFeature> features_;
  analysis::AnalysisPath top1_;
  analysis::AnalyzerImpl* analyzer_;
  std::vector<u32> featureBuf_;
  std::vector<analysis::ConnectionBeamElement> tmpBeam_;
  util::FlatSet<u32> goodFeatures_;
  util::FlatSet<u32> badFeatures_;
  float loss_;
  u32 mask_;
  util::memory::Manager memMgr_;
  std::unique_ptr<util::memory::PoolAlloc> alloc_{memMgr_.core()};

  void handleBoundaryConstraints();
  void handleEos();
  void finalizeFeatures();

  using PtrList = util::ArraySlice<analysis::ConnectionBeamElement>;
  analysis::EntryBeam emptyBeam();
  analysis::EntryBeam candidatesEndingOn(i32 boundary, i32 beforeBoundary,
                                         analysis::EntryBeam entryBeam);
  analysis::EntryBeam candidatesStartingOn(i32 boundary,
                                           analysis::EntryBeam entryBeam);
  analysis::EntryBeam makeUpCandidatesEndingOn(i32 boundary, i32 beforeBoundary,
                                               analysis::EntryBeam entryBeam);
  void addFeatures(PtrList good, const analysis::ConnectionPtr* bad,
                   const analysis::ConnectionPtr* badPrev,
                   const analysis::ConnectionPtr* badPrev2);

  bool isValid(const analysis::ConnectionPtr& cptr) const;

  friend class TrainerBatch;

 public:
  explicit PartialTrainer(analysis::AnalyzerImpl* ana, u32 mask)
      : analyzer_{ana}, mask_{mask}, memMgr_{64 * 1024} {}

  Status prepare();
  Status compute(const analysis::ScorerDef* sconf);

  float loss() const { return loss_; }
  util::ArraySlice<ScoredFeature> featureDiff() const { return features_; }

  ExampleInfo exampleInfo() const { return example_.exampleInfo(); }

  const PartialExample& example() const { return example_; }
  PartialExample& example() { return example_; }

  void markGold(std::function<void(analysis::LatticeNodePtr)> callback,
                analysis::Lattice* lattice) const;

  analysis::EntryBeam findCandidatesSpanning(i32 boundary);

  void addBiTriCorrections(const analysis::ConnectionPtr* good,
                           const analysis::ConnectionPtr* bad,
                           const analysis::ConnectionPtr* badPrev,
                           const analysis::ConnectionPtr* badPrev2);
};

class OwningPartialTrainer : public ITrainer {
  bool isPrepared_ = false;
  util::Lazy<PartialTrainer> trainer_;
  util::Lazy<analysis::AnalyzerImpl> analyzer_;

  friend class TrainerBatch;

 public:
  Status initialize(const TrainerFullConfig& cfg,
                    const analysis::ScorerDef& scorerDef);

  Status prepare() override {
    if (!isPrepared_) {
      auto s = trainer_.value().prepare();
      isPrepared_ = true;
      return s;
    }
    return Status::Ok();
  }

  void reset() { isPrepared_ = false; }

  Status compute(const analysis::ScorerDef* sconf) override {
    return trainer_.value().compute(sconf);
  }

  float loss() const override { return trainer_.value().loss(); }

  util::ArraySlice<ScoredFeature> featureDiff() const override {
    return trainer_.value().featureDiff();
  }

  ExampleInfo exampleInfo() const override {
    return trainer_.value().exampleInfo();
  }

  const analysis::OutputManager& outputMgr() const override;
  void markGold(
      std::function<void(analysis::LatticeNodePtr)> callback) const override;
  analysis::Lattice* lattice() const override;
  void setGlobalBeam(const GlobalBeamTrainConfig& cfg) override;
};

class PartialExampleReader {
  std::string filename_;
  TrainingIo* tio_;
  util::FlatMap<StringPiece, const TrainingExampleField*> fields_;
  util::FullyMappedFile file_;
  util::CsvReader csv_{'\t', '\0'};
  char32_t noBreakToken_ = U'&';
  std::vector<chars::InputCodepoint> codepts_;

 public:
  Status initialize(TrainingIo* tio, char32_t noBreakToken = U'&');
  Status readExample(PartialExample* result, bool* eof);
  Status openFile(StringPiece filename);
  Status setData(StringPiece data);
};

}  // namespace training
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_PARTIAL_TRAINER_H
