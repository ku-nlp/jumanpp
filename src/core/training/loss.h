//
// Created by Arseny Tolmachev on 2017/03/23.
//

#ifndef JUMANPP_LOSS_H
#define JUMANPP_LOSS_H

#include "core/analysis/analysis_result.h"
#include "core/analysis/analyzer_impl.h"
#include "core/training/gold_example.h"
#include "core/training/trainer_base.h"
#include "training_io.h"

namespace jumanpp {
namespace core {
namespace training {

enum class ComparitionClass { Both, TopOnly, GoldOnly };

struct ComparisonStep {
  ComparitionClass cmpClass;
  float lastGoldScore;
  float violation;
  float mismatchWeight;
  i32 numMismatches;

  i32 boundary;
  const analysis::ConnectionBeamElement* topPtr;
  i32 goldPosition;
  i32 numGold;

  bool goldInBeam;

  ComparisonStep(ComparitionClass clz) : cmpClass{clz} {}

  static ComparisonStep both() {
    ComparisonStep step{ComparitionClass::Both};
    return step;
  }

  static ComparisonStep topOnly() {
    ComparisonStep step{ComparitionClass::TopOnly};
    step.mismatchWeight = 0;
    step.numMismatches = 0;
    step.goldInBeam = false;
    return step;
  }

  static ComparisonStep goldOnly() {
    ComparisonStep step{ComparitionClass::GoldOnly};
    step.mismatchWeight = 0;
    step.numMismatches = 0;
    step.violation = 0;
    return step;
  }

  bool hasError() const {
    return cmpClass != ComparitionClass::Both || violation > 0 ||
           numMismatches > 0;
  }
};

class LossCalculator {
  analysis::AnalysisPath top1;
  analysis::AnalyzerImpl* analyzer;
  const spec::TrainingSpec* spec;
  std::vector<u32> rawGoldFeatures;
  std::vector<float> goldScores;
  std::vector<float> goldNodeScores;
  std::vector<u32> top1Features;
  std::vector<u32> goldFeatures;
  std::vector<ScoredFeature> scored;
  std::vector<u32> featureBuffer;
  std::vector<ComparisonStep> comparison;
  GoldenPath gold;
  float fullWeight;

  void mergeOne(u32 target, float score);

 public:
  LossCalculator(analysis::AnalyzerImpl* analyzer,
                 const spec::TrainingSpec* spec)
      : analyzer{analyzer}, spec{spec} {
    featureBuffer.resize(analyzer->core().spec().features.ngram.size());

    fullWeight = 0;
    for (auto& x : spec->fields) {
      fullWeight += x.weight;
    }
  }

  Status resolveTop1() { return top1.fillIn(analyzer->lattice()); }

  Status resolveGold();

  void computeGoldScores(const analysis::ScorerDef* scores);

  bool findWorstTopNode(i32 goldPos, ComparisonStep* step);

  bool isGoldStillInBeam(util::ArraySlice<analysis::ConnectionBeamElement> beam,
                         i32 goldPos);

  Status computeComparison();

  void computeFeatureDiff(u32 mask);

  i32 fullSize() const { return comparison.size(); }

  /**
   *
   * @return index of comparison position where golden example falls off the
   * beam
   */
  i32 fallOffBeam() const {
    auto sz = fullSize();
    for (int i = 0; i < sz; ++i) {
      auto& cmp = comparison[i];
      if (util::contains({ComparitionClass::Both, ComparitionClass::GoldOnly},
                         cmp.cmpClass)) {
        if (!cmp.goldInBeam) {
          return std::min(i + 2, sz - 1);
        }
      }
    }
    return sz;
  }

  i32 maxViolation() const {
    i32 val = 0;
    auto sz = fullSize();
    float viol = 0;
    for (int i = 0; i < sz; ++i) {
      auto& cmp = comparison[i];
      if (util::contains({ComparitionClass::Both, ComparitionClass::TopOnly},
                         cmp.cmpClass)) {
        if (cmp.violation > viol) {
          val = i;
          viol = cmp.violation;
        }
      }
    }
    return std::min(val + 2, sz - 1);
  }

  void computeNgrams(i32 cmpIdx);
  void computeGoldNgrams(i32 cmpIdx, i32 numGold);

  float computeLoss(i32 till);

  util::ArraySlice<ScoredFeature> featureDiff() const { return scored; }

  GoldenPath& goldPath() { return gold; }
  const GoldenPath& goldPath() const { return gold; }
};

}  // namespace training
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_LOSS_H
