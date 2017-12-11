//
// Created by Arseny Tolmachev on 2017/12/08.
//

#ifndef JUMANPP_FEATURE_COMPUTER_H
#define JUMANPP_FEATURE_COMPUTER_H

#include "core/analysis/lattice_config.h"
#include "core/features_api.h"

namespace jumanpp {
namespace core {
namespace features {

enum class NgramSubset {
  Unigrams = 1,
  Bigrams = 2,
  Trigrams = 4,
  UniBi = Unigrams | Bigrams,
  BiTri = Bigrams | Trigrams,
  All = Unigrams | Bigrams | Trigrams
};

struct NgramFeatureRef {
  analysis::LatticeNodePtr t2;
  analysis::LatticeNodePtr t1;
  analysis::LatticeNodePtr t0;

  static NgramFeatureRef init(analysis::LatticeNodePtr ptr) {
    return {{0, 0},  // BOS0
            {1, 0},  // BOS1
            ptr};
  }

  NgramFeatureRef next(analysis::LatticeNodePtr ptr) { return {t1, t0, ptr}; }
  NgramFeatureRef prev(analysis::LatticeNodePtr ptr) { return {ptr, t2, t1}; }
};

class NgramFeaturesComputer {
  const analysis::Lattice* lattice;
  const features::FeatureHolder& features;

 public:
  NgramFeaturesComputer(const analysis::Lattice* lattice,
                        const features::FeatureHolder& features)
      : lattice(lattice), features(features) {}

  void calculateNgramFeatures(const NgramFeatureRef& ptrs,
                              util::MutableArraySlice<u32> result);

  util::ArraySlice<u32> subset(util::ArraySlice<u32> original,
                               NgramSubset what);
};

}  // namespace features
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_FEATURE_COMPUTER_H
