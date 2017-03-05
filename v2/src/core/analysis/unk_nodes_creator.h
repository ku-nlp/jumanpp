//
// Created by Arseny Tolmachev on 2017/03/01.
//

#ifndef JUMANPP_UNK_NODES_CREATOR_H
#define JUMANPP_UNK_NODES_CREATOR_H

#include "core/analysis/analysis_input.h"
#include "core/analysis/dic_reader.h"
#include "core/analysis/extra_nodes.h"
#include "core/analysis/lattice_builder.h"
#include "core/analysis/unk_nodes.h"
#include "core/dic_entries.h"
#include "util/characters.h"

namespace jumanpp {
namespace core {
namespace analysis {

struct UnkNodeConfig {
  OwningDictNode base;
  std::vector<i32> replaceWithSurface;
  i32 notPrefixIndex = -1;

  void fillElems(util::MutableArraySlice<i32> buffer, i32 data) const {
    for (auto idx : replaceWithSurface) {
      buffer.at(idx) = data;
    }
  }

  UnkNodeConfig(const OwningDictNode& base) : base(base) {}
};

class UnkNodesContext {
  ExtraNodesContext* xtra_;

 public:
  UnkNodesContext(ExtraNodesContext* xtra) : xtra_{xtra} {}

  EntryPtr makePtr(StringPiece surface, const UnkNodeConfig& conf,
                   bool notPrefix) {
    auto node = xtra_->makeUnk(conf.base);
    auto data = xtra_->nodeContent(node);
    auto sptr = xtra_->pointerFor(surface);
    conf.fillElems(data, sptr);
    return node->ptr();
  }
};

class ChunkingUnkMaker : public UnkMaker {
  dic::DictionaryEntries entries_;
  chars::CharacterClass charClass_;
  UnkNodeConfig info_;

 public:
  ChunkingUnkMaker(const dic::DictionaryEntries& entries_,
                   chars::CharacterClass charClass_, UnkNodeConfig&& info_);

  bool spawnNodes(const AnalysisInput& input, UnkNodesContext* ctx,
                  LatticeBuilder* lattice) const override;
};

class SingleUnkMaker : public UnkMaker {
  dic::DictionaryEntries entries_;
  chars::CharacterClass charClass_;
  UnkNodeConfig info_;

 public:
  SingleUnkMaker(const dic::DictionaryEntries& entries_,
                 chars::CharacterClass charClass_, UnkNodeConfig&& info_);

  bool spawnNodes(const AnalysisInput& input, UnkNodesContext* ctx,
                  LatticeBuilder* lattice) const override;
};

}  // analysis
}  // core
}  // jumanpp

#endif  // JUMANPP_UNK_NODES_CREATOR_H
