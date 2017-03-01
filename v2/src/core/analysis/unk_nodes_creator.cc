//
// Created by Arseny Tolmachev on 2017/03/01.
//

#include "unk_nodes_creator.h"

namespace jumanpp {
namespace core {
namespace analysis {

bool ChunkingUnkMaker::spawnNodes(const AnalysisInput &input, UnkNodesContext *ctx, LatticeBuilder *lattice) const {
  auto& codepoints = input.codepoints();
  for (LatticePosition i = 0; i < codepoints.size(); ++i) {
    auto& codept = codepoints[i];
    if (!codept.hasClass(charClass_)) {
      continue;
    }

    auto trav = entries_.traversal();
    for (LatticePosition j = i; j < codepoints.size(); ++j) {
      auto& cp = codepoints[j];
      if (!cp.hasClass(charClass_)) {
        break;
      }
      auto status = trav.step(cp.bytes);
      bool havePrefixInDic = true;
      switch (status) {
        case TraverseStatus::NoNode:
          havePrefixInDic = false;
        case TraverseStatus::NoLeaf: {
          LatticePosition start = i;
          LatticePosition end = (LatticePosition) (j + 1);
          auto ptr =
              ctx->makePtr(input.surface(start, end), *info_, havePrefixInDic);
          lattice->appendSeed(ptr, start, end);
          break;
        }
        case TraverseStatus::Ok:
          continue;
      }
    }
  }
  return true;
}

}  // analysis
}  // core
}  // jumanpp
