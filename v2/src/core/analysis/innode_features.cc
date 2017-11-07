//
// Created by Arseny Tolmachev on 2017/11/07.
//

#include "innode_features.h"

namespace jumanpp {
namespace core {
namespace analysis {

bool InNodeFeatureComputer::importOneEntry(
    NodeInfo nfo, util::MutableArraySlice<i32> result) {
  auto ptr = nfo.entryPtr();
  if (ptr.isSpecial()) {
    auto node = xtra_.node(ptr);
    if (node->header.type == ExtraNodeType::Unknown) {
      auto nodeData = xtra_.nodeContent(node);
      util::copy_buffer(nodeData, result);
      auto hash = node->header.unk.contentHash;
      for (auto& e : result) {
        if (e < 0) {
          e = hash;
        }
      }
    } else if (node->header.type == ExtraNodeType::Alias) {
      auto nodeData = xtra_.nodeContent(node);
      util::copy_buffer(nodeData, result);
    } else {
      return false;
    }
  } else {  // dic node
    entries_.entryAtPtr(ptr.dicPtr()).fill(result, result.size());
  }
  return true;
}
}  // namespace analysis
}  // namespace core
}  // namespace jumanpp