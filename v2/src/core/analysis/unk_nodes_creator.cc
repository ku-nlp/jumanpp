//
// Created by Arseny Tolmachev on 2017/03/01.
//

#include "unk_nodes_creator.h"

namespace jumanpp {
namespace core {
namespace analysis {

ChunkingUnkMaker::ChunkingUnkMaker(const dic::DictionaryEntries& entries_,
                                   chars::CharacterClass charClass_,
                                   UnkNodeConfig&& info_)
    : entries_(entries_), charClass_(charClass_), info_(info_) {}

bool ChunkingUnkMaker::spawnNodes(const AnalysisInput& input,
                                  UnkNodesContext* ctx,
                                  LatticeBuilder* lattice) const {
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

      switch (status) {
        case TraverseStatus::NoNode: {
          for (; j < codepoints.size(); ++j) {
            if (!codepoints[j].hasClass(charClass_)) {
              j = codepoints.size();
              break;
            }
            LatticePosition start = i;
            LatticePosition end = (LatticePosition)(j + 1);
            auto ptr = ctx->makePtr(input.surface(start, end), info_, true);
            lattice->appendSeed(ptr, start, end);
          }
          break;
        }
        case TraverseStatus::NoLeaf: {
          LatticePosition start = i;
          LatticePosition end = (LatticePosition)(j + 1);
          auto ptr = ctx->makePtr(input.surface(start, end), info_, false);
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

SingleUnkMaker::SingleUnkMaker(const dic::DictionaryEntries& entries_,
                               chars::CharacterClass charClass_,
                               UnkNodeConfig&& info_)
    : entries_{entries_}, charClass_{charClass_}, info_{info_} {}

bool SingleUnkMaker::spawnNodes(const AnalysisInput& input,
                                UnkNodesContext* ctx,
                                LatticeBuilder* lattice) const {
  auto& codepoints = input.codepoints();
  for (LatticePosition i = 0; i < codepoints.size(); ++i) {
    auto& codept = codepoints[i];
    if (!codept.hasClass(charClass_)) {
      continue;
    } else {
      return false;  // TODO: fix
    }
  }
  return true;
}
}  // analysis
}  // core
}  // jumanpp
