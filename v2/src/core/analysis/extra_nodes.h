//
// Created by Arseny Tolmachev on 2017/02/27.
//

#ifndef JUMANPP_EXTRA_NODES_H
#define JUMANPP_EXTRA_NODES_H

#include <util/flatmap.h>
#include "core/analysis/dic_reader.h"
#include "core/core_types.h"
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
  i32 contentHash;
  StringPiece surface;
};

struct ExtraNodeHeader {
  i32 index = std::numeric_limits<i32>::min();
  ExtraNodeType type = ExtraNodeType::Invalid;
  union {
    AliasNodeHeader alias;
    UnkNodeHeader unk;
  };
};

struct ExtraNode {
  ExtraNodeHeader header;
  i32 content[];

  EntryPtr ptr() const { return EntryPtr{~header.index}; }

  ExtraNode(): header{} {}
};

class ExtraNodesContext {
  size_t numFields_;
  size_t numPlaceholders_;
  util::memory::PoolAlloc* alloc_;
  std::vector<ExtraNode*> extraNodes_;
  util::FlatMap<StringPiece, i32> stringPtrs_;

  ExtraNode* allocateExtra();

 public:
  ExtraNodesContext(util::memory::PoolAlloc* alloc, i32 numFields,
                    i32 numPlaceholders)
      : numFields_{(size_t)numFields},
        numPlaceholders_{(size_t)numPlaceholders},
        alloc_{alloc} {}

  ExtraNode* makeZeroedUnk();
  ExtraNode* makeUnk(const DictNode& pat);
  ExtraNode* makeAlias();

  util::MutableArraySlice<i32> nodeContent(ExtraNode* ptr) const {
    return util::MutableArraySlice<i32>{ptr->content, numFields_};
  }

  util::ArraySlice<i32> nodeContent(ExtraNode const* ptr) const {
    return util::ArraySlice<i32>{ptr->content, numFields_};
  }

  util::MutableArraySlice<i32> aliasBuffer(ExtraNode* node, size_t numNodes);

  const ExtraNode* node(EntryPtr ptr) const {
    JPP_DCHECK(ptr.isSpecial());
    if (ptr.isDic()) {
      return nullptr;
    }
    auto idx = ptr.extPtr();
    JPP_DCHECK_IN(idx, 0, extraNodes_.size());
    return extraNodes_[idx];
  }

  i32 lengthOf(EntryPtr eptr) {
    JPP_DCHECK(eptr.isSpecial());
    auto n = node(eptr);
    return n->header.unk.surface.size();
  }

  void putPlaceholderData(EntryPtr ptr, i32 idx, i32 value) {
    JPP_DCHECK(ptr.isSpecial());
    ExtraNode* n = extraNodes_[ptr.extPtr()];
    putPlaceholderData(n, idx, value);
  }

  void putPlaceholderData(ExtraNode* node, i32 idx, i32 value) {
    JPP_DCHECK_IN(idx, 0, numPlaceholders_);
    node->content[numFields_ + idx] = value;
  }

  i32 placeholderData(EntryPtr ptr, i32 idx) const {
    JPP_DCHECK(ptr.isSpecial());
    JPP_DCHECK_IN(idx, 0, numPlaceholders_);
    ExtraNode* n = extraNodes_[ptr.extPtr()];
    return n->content[numFields_ + idx];
  }

  void reset() {
    extraNodes_.clear();
    stringPtrs_.clear();
  }

  /**
   * Copy this string internally and register it, so it will be usable as a
   * pointer.
   * \see pointerFor
   * @param piece
   * @return
   */
  StringPiece intern(StringPiece piece) {
    auto it = stringPtrs_.find(piece);
    if (it == stringPtrs_.end()) {
      auto ptr = alloc_->allocateArray<char>(piece.size());
      std::copy(piece.begin(), piece.end(), ptr);
      StringPiece copied{ptr, ptr + piece.size()};
      stringPtrs_[copied] = 0;
      return copied;
    }
    return it->first;
  }

  StringPiece unkString(i32 i) const;
};

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_EXTRA_NODES_H
