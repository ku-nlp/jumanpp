//
// Created by Arseny Tolmachev on 2017/03/01.
//

#ifndef JUMANPP_OUTPUT_H
#define JUMANPP_OUTPUT_H

#include "core/analysis/lattice_config.h"
#include "core/core_types.h"
#include "core/dic/dic_entries.h"
#include "core/dic/field_reader.h"
#include "util/array_slice.h"
#include "util/lazy.h"
#include "util/memory.hpp"
#include "util/string_piece.h"

namespace jumanpp {
namespace core {
namespace dic {
class DictionaryHolder;
}

namespace analysis {

class ExtraNodesContext;

enum class NodeLookupStatus { Failure, Single, Multiple };

class OutputManager;

class NodeWalker {
  dic::DicEntryBuffer buffer_;
  const OutputManager* mgr_;
  EntryPtr current_ = EntryPtr::Invalid();

  friend class OutputManager;

 public:
  NodeWalker() = default;

  i32 remaining() const { return buffer_.remaining(); }

  bool next() { return buffer_.nextData(); }

  bool valueOf(i32 fieldIdx, i32* result) const {
    if (!isSuccess()) return false;
    if (fieldIdx < buffer_.numFeatures()) {
      *result = buffer_.features().at(fieldIdx);
      return true;
    }
    fieldIdx -= buffer_.numFeatures();
    if (fieldIdx < buffer_.numData()) {
      *result = buffer_.data().at(fieldIdx);
      return true;
    }
    return false;
  }

  EntryPtr eptr() const { return current_; }

  bool isSuccess() const { return current_ != EntryPtr::Invalid(); }
};

class StringField {
  i32 index_;
  const ExtraNodesContext* xtra_;
  util::Lazy<dic::impl::StringStorageReader> reader_;

  void initialize(i32 index, const ExtraNodesContext* xtra,
                  dic::impl::StringStorageReader reader);

  friend class OutputManager;

 public:
  StringField() = default;
  StringField(const StringField&) = delete;
  StringPiece operator[](const NodeWalker& node) const;
  i32 pointer(const NodeWalker& node) const;
};

class StringListIterator {
  const dic::impl::StringStorageReader& reader_;
  dic::impl::IntListTraversal elements_;
  i32 current_;

 public:
  StringListIterator(const dic::impl::StringStorageReader& reader_,
                     const dic::impl::IntListTraversal& elements_)
      : reader_(reader_), elements_(elements_), current_{0} {}

  bool next(StringPiece* result) {
    bool status = elements_.readOneCumulative(&current_);
    if (status) {
      return reader_.readAt(current_, result);
    }
    return false;
  }

  bool hasNext() const { return elements_.remaining() == 0; }
};

class KVListIterator {
  const dic::impl::StringStorageReader& reader_;
  dic::impl::KeyValueListTraversal elems_;

 public:
  KVListIterator(const dic::impl::StringStorageReader& reader,
                 const dic::impl::KeyValueListTraversal& elems)
      : reader_(reader), elems_(elems) {}

  bool next() noexcept { return elems_.moveNext(); }

  StringPiece key() const noexcept {
    StringPiece sp;
    if (JPP_LIKELY(reader_.readAt(elems_.key(), &sp))) {
      return sp;
    }
    return StringPiece{"!!!!!KVITER_KEY_NOT_FOUND!!!!!"};
  }

  bool hasValue() const noexcept { return elems_.hasValue(); }

  StringPiece value() const noexcept {
    StringPiece sp;
    JPP_DCHECK(hasValue());
    if (JPP_UNLIKELY(!hasValue())) {
      return StringPiece{"!!!!!KVITER_HAS_NO_VALUE!!!!"};
    }
    if (JPP_LIKELY(reader_.readAt(elems_.value(), &sp))) {
      return sp;
    }
    return StringPiece{"!!!!!KVITER_VALUE_NOT_FOUND!!!!!"};
  }

  bool hasNext() const noexcept { return elems_.hasNext(); }
};

class StringListField {
  i32 index_ = -1;
  util::Lazy<dic::impl::IntStorageReader> ints_;
  util::Lazy<dic::impl::StringStorageReader> strings_;

  friend class OutputManager;

  void initialize(i32 index, const dic::impl::IntStorageReader& ints,
                  const dic::impl::StringStorageReader& strings);

 public:
  StringListField() = default;
  StringListField(const StringListField&) = delete;
  StringListIterator operator[](const NodeWalker& node) const;
};

class KVListField {
  i32 index_ = -1;
  util::Lazy<dic::impl::IntStorageReader> ints_;
  util::Lazy<dic::impl::StringStorageReader> strings_;

  friend class OutputManager;

  void initialize(i32 index, const dic::impl::IntStorageReader& ints,
                  const dic::impl::StringStorageReader& strings);

 public:
  KVListField() = default;
  KVListField(const StringListField&) = delete;
  KVListIterator operator[](const NodeWalker& node) const;
};

class OutputManager {
  const ExtraNodesContext* xtra_;
  const dic::DictionaryHolder* holder_;
  dic::DictionaryEntries entries_;
  const Lattice* lattice_;

  friend class NodeWalker;

 public:
  OutputManager(const ExtraNodesContext* xtra,
                const dic::DictionaryHolder* holder, const Lattice* lattice);

  bool locate(LatticeNodePtr ptr, NodeWalker* result) const;
  bool locate(EntryPtr ptr, NodeWalker* result) const;
  NodeWalker nodeWalker() const;
  Status stringField(StringPiece name, StringField* result) const;
  Status stringListField(StringPiece name, StringListField* result) const;
  Status kvListField(StringPiece name, KVListField* result) const;
  const dic::DictionaryHolder& dic() const { return *holder_; }
  i32 valueOfUnkPlaceholder(EntryPtr eptr, i32 placeholderIdx) const;
};

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_OUTPUT_H
