//
// Created by Arseny Tolmachev on 2017/12/08.
//

#include "feature_computer.h"
#include "core/analysis/lattice_types.h"
#include "core/impl/feature_impl_types.h"

namespace jumanpp {
namespace core {
namespace features {

void NgramFeaturesComputer::calculateNgramFeatures(
    const NgramFeatureRef& ptrs, util::MutableArraySlice<u32> result) {
  util::Sliceable<u32> resSlice{result, result.size(), 1};
  auto t2f = lattice->boundary(ptrs.t2.boundary)
                 ->starts()
                 ->patternFeatureData()
                 .row(ptrs.t2.position);
  auto t1f = lattice->boundary(ptrs.t1.boundary)
                 ->starts()
                 ->patternFeatureData()
                 .row(ptrs.t1.position);
  auto t0f = lattice->boundary(ptrs.t0.boundary)
                 ->starts()
                 ->patternFeatureData()
                 .rows(ptrs.t0.position, ptrs.t0.position + 1);

  features::impl::NgramFeatureData nfd{resSlice, t2f, t1f, t0f};
  features.ngram->applyBatch(&nfd);
}

util::ArraySlice<u32> NgramFeaturesComputer::subset(
    util::ArraySlice<u32> original, NgramSubset what) {
  auto& x = features.ngramPartial;
  JPP_DCHECK_EQ(original.size(),
                x->numUnigrams() + x->numBigrams() + x->numTrigrams());
  switch (what) {
    case NgramSubset::Unigrams:
      return {original, 0, x->numUnigrams()};
    case NgramSubset::Bigrams:
      return {original, x->numUnigrams(), x->numBigrams()};
    case NgramSubset::Trigrams:
      return {original, x->numUnigrams() + x->numBigrams(), x->numTrigrams()};
    case NgramSubset::UniBi:
      return {original, 0, x->numUnigrams() + x->numBigrams()};
    case NgramSubset::BiTri:
      return {original, x->numUnigrams(), x->numBigrams() + x->numTrigrams()};
    default:
      JPP_DCHECK_NOT("should not reach here");
      return {};
  }
}

}  // namespace features
}  // namespace core
}  // namespace jumanpp