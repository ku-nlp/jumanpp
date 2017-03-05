//
// Created by Arseny Tolmachev on 2017/03/01.
//

#ifndef JUMANPP_OUTPUT_H
#define JUMANPP_OUTPUT_H

#include "core/analysis/lattice_config.h"
#include "core/core_types.h"
#include "core/dic_entries.h"
#include "core/impl/field_reader.h"
#include "util/array_slice.h"
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
  NodeLookupStatus status_;
  util::MutableArraySlice<i32> values_;
  util::ArraySlice<i32> nodes_;
  i32 remaining_;
  const OutputManager* mgr_;
  friend class OutputManager;

  bool handleMultiple();

 public:
  NodeWalker(util::MutableArraySlice<i32> v)
      : status_{NodeLookupStatus::Failure}, values_{v}, remaining_{0} {}

  i32 remaining() const { return remaining_; }

  bool next() {
    --remaining_;
    if (remaining_ < 0) return false;
    if (status_ == NodeLookupStatus::Multiple) {
      handleMultiple();
    }
    return remaining_ >= 0;
  }

  bool valueOf(i32 fieldIdx, i32* result) const {
    if (!isSuccess()) return false;
    *result = values_.at(fieldIdx);
    return true;
  }

  bool isSuccess() const { return status_ != NodeLookupStatus::Failure; }
};

class StringField {
  i32 index_;
  const ExtraNodesContext* xtra_;
  union {  // we do not want constructor of this to be called
    dic::impl::StringStorageReader reader_;
  };

  void initialize(i32 index, const ExtraNodesContext* xtra,
                  dic::impl::StringStorageReader reader);

  friend class OutputManager;

 public:
  StringField() {}
  StringField(const StringField&) = delete;
  StringPiece operator[](const NodeWalker& node) const;
};

class OutputManager {
  util::memory::ManagedAllocatorCore* alloc_;
  const ExtraNodesContext* xtra_;
  const dic::DictionaryHolder* holder_;
  dic::DictionaryEntries entries_;
  const Lattice* lattice_;

  bool fillEntry(EntryPtr ptr, util::MutableArraySlice<i32> entries) const;

  friend class NodeWalker;

 public:
  OutputManager(util::memory::ManagedAllocatorCore* alloc,
                const ExtraNodesContext* xtra,
                const dic::DictionaryHolder* holder, const Lattice* lattice);

  bool locate(LatticeNodePtr ptr, NodeWalker* result) const;
  bool locate(EntryPtr ptr, NodeWalker* result) const;
  NodeWalker nodeWalker() const;
  Status stringField(StringPiece name, StringField* result) const;
};

}  // analysis
}  // core
}  // jumanpp

#endif  // JUMANPP_OUTPUT_H
