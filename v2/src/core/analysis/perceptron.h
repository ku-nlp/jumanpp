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

inline float computeUnrolled4Perceptron(WeightBuffer weights,
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

// This guy does not do masking
inline float computeUnrolled4RawPerceptron(
    WeightBuffer weights, const util::ArraySlice<u32> indices) {
  // basically the sole purpose of this unrolling
  // is to be able to do several parallel memory fetches at once
  float r1 = 0, r2 = 0, r3 = 0, r4 = 0;
  int i;
  for (i = 0; (i + 4) <= indices.size(); i += 4) {
    r1 += weights.at(indices.at(i));
    r2 += weights.at(indices.at(i + 1));
    r3 += weights.at(indices.at(i + 2));
    r4 += weights.at(indices.at(i + 3));
  }
  auto rest = indices.size() - i;
  JPP_DCHECK_IN(rest, 0, 4);
  switch (rest & 0x3) {
    case 3:
      r3 += weights.at(indices.at(i + 2));
    case 2:
      r2 += weights.at(indices.at(i + 1));
    case 1:
      r1 += weights.at(indices.at(i));
    case 0:
    default:;  // noop
  }
  return r1 + r2 + r3 + r4;
}
}  // namespace impl

class PerceptronState;

class HashedFeaturePerceptron : public FeatureScorer {
  util::ArraySlice<float> weights_;
  std::unique_ptr<PerceptronState> state_;

 public:
  HashedFeaturePerceptron();
  HashedFeaturePerceptron(const util::ArraySlice<float>& weights);
  ~HashedFeaturePerceptron();

  void compute(util::MutableArraySlice<float> result,
               util::ConstSliceable<u32> ngrams) const override;
  void add(util::ArraySlice<float> source,
           util::MutableArraySlice<float> result,
           util::ConstSliceable<u32> features) const override;
  Status load(const model::ModelInfo& model) override;

  void setWeightsTo(util::ArraySlice<float> weights) { weights_ = weights; }

  WeightBuffer weights() const override;
};

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_PERCEPTRON_H
