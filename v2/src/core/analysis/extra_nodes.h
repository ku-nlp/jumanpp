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
};

struct ExtraNodeHeader {
  i32 index;
  ExtraNodeType type;
  union {
    AliasNodeHeader alias;
    UnkNodeHeader unk;
  };
};

struct ExtraNode {
  ExtraNodeHeader header;
  i32 content[];

  EntryPtr ptr() const { return EntryPtr{~header.index}; }
};

class ExtraNodesContext {
  size_t numFields_;
  size_t numPlaceholders_;
  util::memory::ManagedAllocatorCore* alloc_;
  std::vector<ExtraNode*> extraNodes_;
  util::FlatMap<StringPiece, i32> stringPtrs_;
  std::vector<StringPiece> strings_;

  ExtraNode* allocateExtra() {
    size_t memory =
        sizeof(ExtraNodeHeader) + sizeof(i32) * (numFields_ + numPlaceholders_);
    void* rawPtr = alloc_->allocate_memory(memory, alignof(ExtraNode));
    auto ptr = reinterpret_cast<ExtraNode*>(rawPtr);
    ptr->header.index = (i32)extraNodes_.size();
    extraNodes_.push_back(ptr);
    return ptr;
  }

 public:
  ExtraNodesContext(util::memory::ManagedAllocatorCore* alloc, i32 numFields,
                    i32 numPlaceholders)
      : numFields_{(size_t)numFields},
        numPlaceholders_{(size_t)numPlaceholders},
        alloc_{alloc} {}

  ExtraNode* makeUnk(const DictNode& pat);

  util::MutableArraySlice<i32> nodeContent(ExtraNode* ptr) const {
    return util::MutableArraySlice<i32>{ptr->content, numFields_};
  }

  util::ArraySlice<i32> nodeContent(ExtraNode const* ptr) const {
    return util::ArraySlice<i32>{ptr->content, numFields_};
  }

  const ExtraNode* node(EntryPtr ptr) const {
    if (ptr.isDic()) {
      return nullptr;
    }
    auto idx = ptr.extPtr();
    JPP_DCHECK_IN(idx, 0, extraNodes_.size());
    return extraNodes_[idx];
  }

  i32 lengthOf(EntryPtr eptr) {
    JPP_DCHECK(eptr.isSpecial());
    i32 realPtr = eptr.extPtr();
    JPP_DCHECK_IN(realPtr, 0, strings_.size());
    auto sp = strings_[realPtr];
    return (i32)sp.size();
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
    strings_.clear();
    stringPtrs_.clear();
  }

  /**
   * Register this string, so it would be usable as a pointer.
   * This version does not intern content of a string, simply assings an id to
   * one
   * \see intern
   * @param piece
   * @return
   */
  i32 pointerFor(StringPiece piece) {
    auto it = stringPtrs_.find(piece);
    if (it == stringPtrs_.end()) {
      i32 idx = ~static_cast<i32>(strings_.size());
      stringPtrs_[piece] = idx;
      strings_.push_back(piece);
      return idx;
    } else {
      return it->second;
    }
  }

  /**
   * Copy this string internally and register it, so it will be usable as a
   * pointer.
   * \see pointerFor
   * @param piece
   * @return
   */
  i32 intern(StringPiece piece) {
    auto it = stringPtrs_.find(piece);
    if (it == stringPtrs_.end()) {
      auto ptr = alloc_->allocateArray<char>(piece.size());
      std::copy(piece.begin(), piece.end(), ptr);
      return pointerFor(StringPiece{
          reinterpret_cast<StringPiece::pointer_t>(ptr), piece.size()});
    }
    return it->second;
  }

  StringPiece string(i32 ptr) const {
    i32 actual = ~ptr;
    JPP_DCHECK_IN(actual, 0, strings_.size());
    return strings_[actual];
  }
};

}  // analysis
}  // core
}  // jumanpp

#endif  // JUMANPP_EXTRA_NODES_H
