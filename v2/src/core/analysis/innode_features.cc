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
}  // namespace analysis
}  // namespace core
}  // namespace jumanpp