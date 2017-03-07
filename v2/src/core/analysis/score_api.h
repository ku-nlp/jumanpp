//
// Created by Arseny Tolmachev on 2017/03/07.
//

#ifndef JUMANPP_SCORE_API_H
#define JUMANPP_SCORE_API_H

#include <util/sliceable_array.h>
#include "core/analysis/lattice_config.h"
#include "core/impl/model_format.h"
#include "util/array_slice.h"
#include "util/status.hpp"

namespace jumanpp {
namespace core {
namespace analysis {

class Lattice;
class LatticeBoundary;
class LatticeBoundaryConnection;
class LatticePlugin;

class ScoreData {
  const Lattice* lattice_;
  const LatticeBoundary* boundary_;
  LatticeBoundaryConnection* connection_;
  util::Sliceable<u32> features_;
  util::ArraySlice<ConnectionPtr> t1Beam_;

 public:
  ScoreData(const Lattice* lattice_, const LatticeBoundary* boundary_,
            LatticeBoundaryConnection* connection_,
            const util::Sliceable<u32>& features_,
            const util::ArraySlice<ConnectionPtr>& t1Beam_)
      : lattice_(lattice_),
        boundary_(boundary_),
        connection_(connection_),
        features_(features_),
        t1Beam_(t1Beam_) {}

  const Lattice* lattice() const { return lattice_; }
  const LatticeBoundary* boundary() const { return boundary_; }
  LatticeBoundaryConnection* connection() { return connection_; }
  const util::Sliceable<u32>& features() { return features_; }
  util::ArraySlice<ConnectionPtr> t1Beam() { return t1Beam_; }
};

class ScorerBase {
 public:
  virtual ~ScorerBase() = default;
  virtual Status load(const model::ModelInfo& model) = 0;
};

class FeatureScorer : public ScorerBase {
 public:
  virtual void compute(util::MutableArraySlice<float> result,
                       util::Sliceable<u32> features) = 0;
};

class ScoreComputer {
 public:
  virtual ~ScoreComputer() = default;
  virtual void compute(util::MutableArraySlice<float> result,
                       ScoreData* data) = 0;
  virtual LatticePlugin* plugin() const { return nullptr; }
};

struct ScoreConfig {
  FeatureScorer* feature;
  std::vector<ScoreComputer> others;
  std::vector<Score> scoreWeights;
};

}  // analysis
}  // core
}  // jumanpp

#endif  // JUMANPP_SCORE_API_H
