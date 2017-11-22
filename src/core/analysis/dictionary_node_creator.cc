//
// Created by Arseny Tolmachev on 2017/02/23.
//

#include "dictionary_node_creator.h"

namespace jumanpp {
namespace core {
namespace analysis {

bool DictionaryNodeCreator::spawnNodes(const AnalysisInput& input,
                                       LatticeBuilder* lattice) {
  auto& points = input.codepoints();

  LatticePosition totalPoints = (LatticePosition)points.size();

  for (LatticePosition begin = 0; begin < totalPoints; ++begin) {
    auto trav = entries_.traversal();

    using dic::TraverseStatus;

    for (LatticePosition position = begin; position < totalPoints; ++position) {
      auto& cp = points[position];
      TraverseStatus status = trav.step(cp.bytes);
      if (status == TraverseStatus::Ok) {
        LatticePosition end = position + LatticePosition(1);
        auto dicEntries = trav.entries();
        while (dicEntries.readOnePtr()) {
          lattice->appendSeed(dicEntries.currentPtr(), begin, end);
        }
      } else if (status == TraverseStatus::NoNode) {
        break;
      }
    }
  }

  return true;
}

DictionaryNodeCreator::DictionaryNodeCreator(
    const dic::DictionaryEntries& entries_)
    : entries_(entries_) {}

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp
