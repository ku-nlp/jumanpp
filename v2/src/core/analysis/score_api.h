//
// Created by Arseny Tolmachev on 2017/03/07.
//

#ifndef JUMANPP_SCORE_API_H
#define JUMANPP_SCORE_API_H

#include <memory>
#include "core/analysis/lattice_config.h"
#include "core/impl/model_format.h"
#include "util/array_slice.h"
#include "util/sliceable_array.h"
#include "util/status.hpp"

namespace jumanpp {
namespace core {
namespace analysis {

class Lattice;
class ExtraNodesContext;

class ScorerBase {
 public:
  virtual ~ScorerBase() = default;
  virtual Status load(const model::ModelInfo& model) = 0;
};

class FeatureScorer : public ScorerBase {
 public:
  virtual void compute(util::MutableArraySlice<float> result,
                       util::ConstSliceable<u32> features) const = 0;
  virtual void add(util::MutableArraySlice<float> result,
                   util::ConstSliceable<u32> features) const = 0;
};

class ScoreComputer {
 public:
  virtual ~ScoreComputer() = default;
  virtual void preScore(Lattice* l, ExtraNodesContext* xtra) = 0;
  virtual bool scoreBoundary(i32 scorerIdx, Lattice* l, i32 boundary) = 0;
};

class ScorerFactory : public ScorerBase {
 public:
  virtual Status makeInstance(std::unique_ptr<ScoreComputer>* result) = 0;
};

struct ScorerDef {
  FeatureScorer* feature;
  std::vector<ScorerFactory*> others;
  std::vector<Score> scoreWeights;
};

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_SCORE_API_H
