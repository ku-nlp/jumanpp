//
// Created by Arseny Tolmachev on 2017/03/01.
//

#include "extra_nodes.h"
#include "core/analysis/dic_reader.h"

namespace jumanpp {
namespace core {
namespace analysis {

ExtraNode *ExtraNodesContext::makeUnk(const DictNode &pat) {
  auto node = allocateExtra();
  node->header.type = ExtraNodeType::Unknown;
  pat.copyTo(node);
  return node;
}

}  // analysis
}  // core
}  // jumanpp