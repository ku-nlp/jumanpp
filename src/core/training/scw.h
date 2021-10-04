//
// Created by Arseny Tolmachev on 2017/03/29.
//

#ifndef JUMANPP_SCW_H
#define JUMANPP_SCW_H

#include <cmath>
#include <vector>
#include "core/analysis/perceptron.h"
#include "core/impl/model_format.h"
#include "core/training/loss.h"
#include "core/training/training_types.h"

namespace jumanpp {
namespace core {
namespace training {

struct ScwData;

/**
 * Structured learning perceptron-like algorithm
 * http://icml.cc/2012/papers/86.pdf
 */
class SoftConfidenceWeighted {
  double phi;
  double C;
  double zeta;
  double psi;

  std::vector<float> usableWeights;
  std::vector<float> matrixDiagonal;

  u32 featureExponent_;
  u32 randomSeed_;

  analysis::HashedFeaturePerceptron perceptron;
  analysis::ScorerDef sconf;
  std::unique_ptr<ScwData> data_;

  void updateMatrix(float beta, util::ArraySlice<ScoredFeature> features);

  double calcAlpha(double vt, double mt);

  double calcBeta(double alphat, double ut, double vt);

  double calcUt(double alphat, double vt);

  double calcVt(util::ArraySlice<ScoredFeature> features);

  double calcScore(util::ArraySlice<ScoredFeature> features);

  void updateWeights(float alpha, float y,
                     util::ArraySlice<ScoredFeature> features);

 public:
  explicit SoftConfidenceWeighted(const TrainingConfig& conf);
  Status validate() const;
  void update(float loss, util::ArraySlice<ScoredFeature> features);
  const analysis::ScorerDef* scorers() const { return &sconf; }
  void exportModel(model::ModelInfo* model, StringPiece comment = EMPTY_SP);
  void dumpModel(StringPiece directory, StringPiece prefix, i32 number);
  u64 substractInitValues();
  u64 numWeights() const { return usableWeights.size(); }
  void setAllWeights(float value);
  ~SoftConfidenceWeighted();
};

}  // namespace training
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_SCW_H
