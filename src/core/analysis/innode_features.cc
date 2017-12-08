//
// Created by Arseny Tolmachev on 2017/11/07.
//

#include "innode_features.h"

namespace jumanpp {
namespace core {
namespace analysis {

bool InNodeFeatureComputer::importOneEntry(
    NodeInfo nfo, util::MutableArraySlice<i32> result) {
  return pfc_.fillEntryBuffer(nfo.entryPtr(), result);
}

void InNodeFeatureComputer::patternFeaturesDynamic(LatticeBoundary *lb) {
  auto nodes = lb->starts();
  auto ptrs = nodes->nodeInfo();
  auto entries = nodes->entryData();
  auto primFeature = nodes->patternFeatureData();
  features::impl::PrimitiveFeatureData pfd{ptrs, entries, primFeature};
  features_.patternDynamic->applyBatch(&pfc_, &pfd);
}

void InNodeFeatureComputer::patternFeaturesEos(Lattice *l) {
  auto eosBnd = l->boundary(l->createdBoundaryCount() - 1);
  auto nodes = eosBnd->starts();
  auto eosSlice = nodes->entryData();
  util::fill(eosSlice, EntryPtr::EOS().rawValue());
  features::impl::PrimitiveFeatureData pfd{nodes->nodeInfo(), eosSlice,
                                           nodes->patternFeatureData()};
  features_.patternDynamic->applyBatch(&pfc_, &pfd);
}
}  // namespace analysis
}  // namespace core
}  // namespace jumanpp