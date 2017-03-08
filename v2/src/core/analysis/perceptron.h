//
// Created by Arseny Tolmachev on 2017/03/07.
//

#ifndef JUMANPP_PERCEPTRON_H
#define JUMANPP_PERCEPTRON_H

#include "score_api.h"

namespace jumanpp {
namespace core {
namespace analysis {

namespace impl {

inline float computeUnrolled4Perceptron(const util::ArraySlice<float> weights,
                                        const util::ArraySlice<u32> indices,
                                        u32 mask) {
  // basically the sole purpose of this unrolling
  // is to be able to do several parallel memory fetches at once
  float r1 = 0, r2 = 0, r3 = 0, r4 = 0;
  int i;
  for (i = 0; (i + 4) <= indices.size(); i += 4) {
    r1 += weights.at(indices.at(i) & mask);
    r2 += weights.at(indices.at(i + 1) & mask);
    r3 += weights.at(indices.at(i + 2) & mask);
    r4 += weights.at(indices.at(i + 3) & mask);
  }
  auto rest = indices.size() - i;
  JPP_DCHECK_IN(rest, 0, 4);
  switch (rest & 0x3) {
    case 3:
      r3 += weights.at(indices.at(i + 2) & mask);
    case 2:
      r2 += weights.at(indices.at(i + 1) & mask);
    case 1:
      r1 += weights.at(indices.at(i) & mask);
    case 0:
    default:;  // noop
  }
  return r1 + r2 + r3 + r4;
}
}

class HashedFeaturePerceptron : public FeatureScorer {
  util::ArraySlice<float> weights_;

 public:
  HashedFeaturePerceptron() {}
  HashedFeaturePerceptron(const util::ArraySlice<float>& weights):
    weights_{weights} {}
  
  void compute(util::MutableArraySlice<float> result,
               util::Sliceable<u32> ngrams) override;
  Status load(const model::ModelInfo &model) override;
};

}  // analysis
}  // core
}  // jumanpp

#endif  // JUMANPP_PERCEPTRON_H
