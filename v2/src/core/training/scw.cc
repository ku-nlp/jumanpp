//
// Created by Arseny Tolmachev on 2017/03/29.
//

#include "scw.h"
#include "util/memory.hpp"
#include <random>

namespace jumanpp {
namespace core {
namespace training {
void SoftConfidenceWeighted::update(float loss,
                                    util::ArraySlice<ScoredFeature> features) {
  if (loss < 1e-5) {
    return;
  }

  double score = calcScore(features);
  double vt = calcVt(features);

  double alphat = calcAlpha(vt, loss * score);
  double ut = calcUt(alphat, vt);
  double betat = calcBeta(alphat, ut, vt);

  JPP_DCHECK(std::isfinite(betat));
  JPP_DCHECK(std::isfinite(alphat));
  if (vt == 0) {
    return;
  }
  updateWeights((float)alphat, loss, features);
  updateMatrix((float)betat, features);
}

void SoftConfidenceWeighted::updateWeights(
    float alpha, float y, util::ArraySlice<ScoredFeature> features) {
  for (auto& v : features) {
    usableWeights[v.feature] += alpha * y * matrixDiagonal[v.feature] * v.score;
  }
}

double SoftConfidenceWeighted::calcScore(
    util::ArraySlice<ScoredFeature> features) {
  double sum = 0;
  for (auto& v : features) {
    sum += usableWeights[v.feature] * v.score;
  }
  return sum;
}

double SoftConfidenceWeighted::calcVt(
    util::ArraySlice<ScoredFeature> features) {
  double sum = 0;
  for (auto& v : features) {
    sum += v.score * v.score * matrixDiagonal[v.feature];
  }
  return sum;
}

double SoftConfidenceWeighted::calcUt(double alphat, double vt) {
  double t = (-alphat * vt * phi +
              std::sqrt(alphat * alphat * vt * vt * phi * phi + 4 * vt));
  return (1.0 / 4.0) * t * t;
}

double SoftConfidenceWeighted::calcBeta(double alphat, double ut, double vt) {
  return (alphat * phi) / (std::sqrt(ut) + (vt * alphat * phi));
}

double SoftConfidenceWeighted::calcAlpha(double vt, double mt) {
  double alpha =
      (1.0f / (vt * zeta)) *
      (-mt * psi + std::sqrt((mt * mt) * (phi * phi * phi * phi / 4.0f) +
                             vt * phi * phi * zeta));
  if (alpha < 0.0) return 0.0;
  if (alpha > C) return C;
  return alpha;
}

void SoftConfidenceWeighted::updateMatrix(
    float beta, util::ArraySlice<ScoredFeature> features) {
  for (auto& f : features) {
    float curValue = matrixDiagonal[f.feature];
    float update = curValue * curValue * f.score * f.score;
    matrixDiagonal[f.feature] -= beta * update;
  }
}

SoftConfidenceWeighted::SoftConfidenceWeighted(const TrainingConfig &conf) :
    phi{conf.scw.phi}, C{conf.scw.C}, zeta { 1 + phi * phi}, psi{1 + phi * phi / 2} {
  usableWeights.reserve(conf.numHashedFeatures);
  matrixDiagonal.resize(conf.numHashedFeatures);
  
  std::default_random_engine eng{conf.randomSeed};
  float boundary = 1.0f / conf.numHashedFeatures;
  std::uniform_real_distribution<float> dist{-boundary, boundary};
  for (int i = 0; i < conf.numHashedFeatures; ++i) {
    usableWeights.push_back(dist(eng));
  }

  perceptron = analysis::HashedFeaturePerceptron(usableWeights);
}

Status SoftConfidenceWeighted::validate() const {
  if (!util::memory::IsPowerOf2(usableWeights.size())) {
    return Status::InvalidParameter() << "number of features must be a power of 2";
  }
  return Status::Ok();
}

}  // training
}  // core
}  // jumanpp