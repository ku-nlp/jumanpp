//
// Created by Arseny Tolmachev on 2017/10/07.
//

#ifndef JUMANPP_PARTIAL_EXAMPLE_H
#define JUMANPP_PARTIAL_EXAMPLE_H

#include <string>
#include <vector>
#include "core/analysis/analysis_result.h"
#include "core/analysis/analyzer_impl.h"
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

struct TagConstraint {
  i32 field;
  i32 value;
};

struct NodeConstraint {
  i32 boundary;
  i32 length;
  std::string surface;
  std::vector<TagConstraint> tags;
};

class PartialExample {
  std::string comment_;
  StringPiece file_;
  i64 line_;
  std::string surface_;
  std::vector<i32> boundaries_;
  std::vector<NodeConstraint> nodes_;

 public:
  i64 line() const { return line_; }

  StringPiece surface() const { return surface_; }

  util::ArraySlice<i32> boundaries() const { return boundaries_; }

  util::ArraySlice<NodeConstraint> nodes() const { return nodes_; }

  ExampleInfo exampleInfo() const {
    return ExampleInfo{file_, comment_, line_};
  }

  bool doesNodeMatch(const analysis::Lattice* l, i32 boundary,
                     i32 position) const;
  bool doesNodeMatch(const analysis::LatticeRightBoundary* lr, i32 boundary,
                     i32 position) const;

  friend class PartialExampleReader;
};

class PartialTrainer {
  PartialExample example_;
  std::vector<ScoredFeature> features_;
  analysis::AnalysisPath top1_;
  analysis::AnalyzerImpl* analyzer_;
  std::vector<u32> featureBuf_;
  float loss_;
  u32 mask_;

  void handleBoundaryConstraints();
  void handleEos();
  void handleTagConstraints();
  void finalizeFeatures();

  friend class TrainerBatch;

 public:
  explicit PartialTrainer(analysis::AnalyzerImpl* ana, u32 mask)
      : analyzer_{ana}, mask_{mask} {}

  Status prepare();
  Status compute(const analysis::ScorerDef* sconf);

  float loss() const { return loss_; }
  util::ArraySlice<ScoredFeature> featureDiff() const { return features_; }

  void addBadNode(const analysis::ConnectionPtr* node, i32 boundary,
                  i32 prevBoundary);
  float addBadNode2(const analysis::ConnectionPtr* node, i32 boundary,
                    i32 length, util::ArraySlice<TagConstraint> tagFilter);

  ExampleInfo exampleInfo() const { return example_.exampleInfo(); }

  const PartialExample& example() const { return example_; }
  PartialExample& example() { return example_; }

  void markGold(std::function<void(analysis::LatticeNodePtr)> callback,
                analysis::Lattice* lattice) const;
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
};

class PartialExampleReader {
  std::string filename_;
  TrainingIo* tio_;
  util::FlatMap<StringPiece, const TrainingExampleField*> fields_;
  util::FullyMappedFile file_;
  util::CsvReader csv_{'\t', '\0'};
  std::vector<chars::InputCodepoint> codepts_;

 public:
  Status initialize(TrainingIo* tio);
  Status readExample(PartialExample* result, bool* eof);
  Status openFile(StringPiece filename);
  Status setData(StringPiece data);
};

}  // namespace training
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_PARTIAL_EXAMPLE_H
