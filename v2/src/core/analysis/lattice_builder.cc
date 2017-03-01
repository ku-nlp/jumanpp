//
// Created by Arseny Tolmachev on 2017/02/23.
//

#include "lattice_builder.h"

namespace jumanpp {
namespace core {
namespace analysis {

Status LatticeBuilder::prepare() {
  std::sort(seeds_.begin(), seeds_.end(),
            [](const LatticeNodeSeed &l, const LatticeNodeSeed &r) -> bool {
              return l.codepointStart < r.codepointStart;
            });

  LatticePosition curStart = 0;
  u16 nodeCount = 0;
  u32 nodeOffset = 0;

  auto prepareNodes = [&](const LatticeNodeSeed &seed) {
    while (curStart != seed.codepointStart) {
      boundaries_.emplace_back(curStart, nodeCount, nodeOffset);
      nodeOffset += nodeCount;
      nodeCount = 0;
      curStart += 1;
    }
  };

  for (auto &seed : seeds_) {
    if (seed.codepointStart != curStart) {
      prepareNodes(seed);
    }
    nodeCount += 1;
  }
  LatticePosition eosPos = maxBoundaries_ + 1;
  LatticeNodeSeed eos{EntryPtr::BOS, eosPos, eosPos};
  prepareNodes(eos);

  if (boundaries_.front().startCount == 0) {
    return Status::InvalidParameter()
           << "could not build lattice, BOS was not connected to anything";
  }
  return Status::Ok();
}

}  // analysis
}  // core
}  // jumanpp