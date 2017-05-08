//
// Created by Arseny Tolmachev on 2017/03/29.
//

#include "scw.h"
#include <cmath>
#include <random>
#include "core/impl/perceptron_io.h"
#include "util/coded_io.h"
#include "util/serialization.h"

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
    auto update = alpha * y * matrixDiagonal[v.feature] * v.score;
    usableWeights[v.feature] += update;
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

SoftConfidenceWeighted::SoftConfidenceWeighted(const TrainingConfig& conf)
    : phi{conf.scw.phi},
      C{conf.scw.C},
      zeta{1 + phi * phi},
      psi{1 + phi * phi / 2} {
  usableWeights.reserve(conf.numHashedFeatures);
  matrixDiagonal.resize(conf.numHashedFeatures, 1.0);

  std::default_random_engine eng{conf.randomSeed};
  float boundary = (float)(1.0 / std::sqrt(conf.numHashedFeatures));
  std::uniform_real_distribution<float> dist{-boundary, boundary};
  for (int i = 0; i < conf.numHashedFeatures; ++i) {
    usableWeights.push_back(dist(eng));
  }

  perceptron = analysis::HashedFeaturePerceptron(usableWeights);
  sconf.feature = &perceptron;
  sconf.scoreWeights.clear();
  sconf.scoreWeights.push_back(1.0f);
}

Status SoftConfidenceWeighted::validate() const {
  if (!util::memory::IsPowerOf2(usableWeights.size())) {
    return Status::InvalidParameter()
           << "number of features must be a power of 2";
  }
  return Status::Ok();
}

struct ScwData {
  util::CodedBuffer cbuf;
};

void SoftConfidenceWeighted::save(model::ModelInfo* model) {
  data_.reset(new ScwData);
  util::serialization::Saver svr{&data_->cbuf};
  PerceptronInfo pi;

  pi.modelSizeExponent = 10;
  svr.save(pi);

  model::ModelPart part;
  part.kind = model::ModelPartKind::Perceprton;
  part.data.push_back(data_->cbuf.contents());

  float* weightsPtr = usableWeights.data();

  auto charPtr = reinterpret_cast<char*>(weightsPtr);
  StringPiece weightsMemory{charPtr,
                            charPtr + usableWeights.size() * sizeof(float)};

  part.data.push_back(weightsMemory);

  model->parts.push_back(std::move(part));
}

SoftConfidenceWeighted::~SoftConfidenceWeighted() {}

}  // namespace training
}  // namespace core
}  // namespace jumanpp