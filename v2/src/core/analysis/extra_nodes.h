//
// Created by Arseny Tolmachev on 2017/02/27.
//

#ifndef JUMANPP_EXTRA_NODES_H
#define JUMANPP_EXTRA_NODES_H

#include "util/array_slice.h"
#include "util/memory.hpp"
#include "util/types.hpp"

namespace jumanpp {
namespace core {
namespace analysis {

enum class ExtraNodeType { Invalid, Unknown, Alias, Special };

struct AliasNodeHeader {
  util::ArraySlice<i32> dictionaryNodes;
};

struct UnkNodeHeader {
  util::ArraySlice<i32> providedValues;
};

struct ExtraNodeHeader {
  ExtraNodeType type;
  union {
    AliasNodeHeader alias;
    UnkNodeHeader unk;
  };
};

struct ExtraNode {
  ExtraNodeHeader header;
  i32 content[];
};

class ExtraNodesContext {
  util::memory::ManagedAllocatorCore* alloc;
  util::memory::ManagedVector<ExtraNode*> extraNodes;

 public:
  const ExtraNode* node(EntryPtr ptr) const {
    if (!isSpecial(ptr)) {
      return nullptr;
    }
    auto idx = idxFromEntryPtr(ptr);
    JPP_DCHECK_IN(idx, 0, extraNodes.size());
    return extraNodes[idx];
  }
};

}  // analysis
}  // core
}  // jumanpp

#endif  // JUMANPP_EXTRA_NODES_H
