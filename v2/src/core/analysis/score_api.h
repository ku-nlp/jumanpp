//
// Created by Arseny Tolmachev on 2017/03/07.
//

#ifndef JUMANPP_SCORE_API_H
#define JUMANPP_SCORE_API_H

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

class ScoreComputer {
 public:
  virtual ~ScoreComputer() = default;
  virtual void compute(util::MutableArraySlice<float> result,
                       const Lattice* lattice, const LatticeBoundary* focus,
                       LatticeBoundaryConnection* connection) = 0;
  virtual LatticePlugin* plugin() const { return nullptr; }
  virtual Status load(const model::ModelInfo& model) = 0;
};

}  // analysis
}  // core
}  // jumanpp

#endif  // JUMANPP_SCORE_API_H
