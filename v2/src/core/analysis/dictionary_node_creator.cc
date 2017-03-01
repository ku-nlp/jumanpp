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

    for (LatticePosition position = begin; position < totalPoints; ++position) {
      auto& cp = points[position];
      TraverseStatus status = trav.step(cp.bytes);
      if (status == TraverseStatus::Ok) {
        LatticePosition end = position + LatticePosition(1);
        auto dicEntries = trav.entries();
        i32 ptr = 0;
        while (dicEntries.readOnePtr(&ptr)) {
          EntryPtr eptr = static_cast<EntryPtr>(ptr);
          lattice->appendSeed(eptr, begin, end);
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

}  // analysis
}  // core
}  // jumanpp
