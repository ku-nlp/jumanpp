//
// Created by Arseny Tolmachev on 2017/03/07.
//

#include "core/analysis/perceptron.h"
#include "core/analysis/lattice_types.h"

namespace jumanpp {
namespace core {
namespace analysis {

void HashedFeaturePerceptron::compute(util::MutableArraySlice<float> result,
                                      util::Sliceable<u32> ngrams) const {
  JPP_DCHECK(util::memory::IsPowerOf2(weights_.size()));
  u32 mask = static_cast<u32>(weights_.size() - 1);
  for (int i = 0; i < ngrams.numRows(); ++i) {
    result.at(i) =
        impl::computeUnrolled4Perceptron(weights_, ngrams.row(i), mask);
  }
}

Status HashedFeaturePerceptron::load(const model::ModelInfo &model) {
  return Status::NotImplemented();
}

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp