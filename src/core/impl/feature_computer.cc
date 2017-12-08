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

}  // namespace features
}  // namespace core
}  // namespace jumanpp