//
// Created by Arseny Tolmachev on 2017/03/01.
//

#ifndef JUMANPP_UNK_NODES_CREATOR_H
#define JUMANPP_UNK_NODES_CREATOR_H

#include "core/analysis/analysis_input.h"
#include "core/dic_entries.h"
#include "core/analysis/extra_nodes.h"
#include "core/analysis/lattice_builder.h"
#include "util/characters.h"
#include "core/analysis/dic_reader.h"

namespace jumanpp {
namespace core {
namespace analysis {

struct UnkNodeConfig {
  DictNode base;
  util::ArraySlice<i32> replaceWithSurface;

  void fillElems(util::MutableArraySlice<i32> buffer, i32 data) const {
    for (auto idx : replaceWithSurface) {
      buffer.at(idx) = data;
    }
  }
};

class UnkNodesContext {
  ExtraNodesContext* xtra_;
 public:
  EntryPtr makePtr(StringPiece surface, const UnkNodeConfig& conf, bool notPrefix) {
    auto node = xtra_->makeUnk(conf.base);
    auto data = xtra_->nodeContent(node);
    auto sptr = xtra_->pointerFor(surface);
    conf.fillElems(data, sptr);
    return node->ptr();
  }
};

class ChunkingUnkMaker {
  dic::DictionaryEntries entries_;
  chars::CharacterClass charClass_;
  UnkNodeConfig* info_;

 public:
  Status initialize(UnkNodesContext* ctx, dic::DictionaryEntries entries);
  bool spawnNodes(const AnalysisInput& input, UnkNodesContext* ctx,
                  LatticeBuilder* lattice) const;
};

}  // analysis
}  // core
}  // jumanpp

#endif  // JUMANPP_UNK_NODES_CREATOR_H
