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

struct UnkNodeHeader {};

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
  util::memory::ManagedVector<ExtraNodeHeader> extraNodes;
};

}  // analysis
}  // core
}  // jumanpp

#endif  // JUMANPP_EXTRA_NODES_H
