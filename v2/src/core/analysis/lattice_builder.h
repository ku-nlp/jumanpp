//
// Created by Arseny Tolmachev on 2017/02/23.
//

#ifndef JUMANPP_LATTICE_BUILDER_H
#define JUMANPP_LATTICE_BUILDER_H

#include "util/types.hpp"

namespace jumanpp {
namespace core {
namespace analysis {

using LatticePosition = u16;

struct LatticeNodeSeed {
  i32 entryPtr;
  LatticePosition codepointStart;
  LatticePosition codepointEnd;
};

class LatticeBuilder {
 public:
  void appendSeed(LatticeNodeSeed seed);

  void reset();
};

}  // analysis
}  // core
}  // jumanpp

#endif  // JUMANPP_LATTICE_CONSTRUCTION_H
