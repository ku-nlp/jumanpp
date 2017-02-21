//
// Created by Arseny Tolmachev on 2017/02/21.
//

#ifndef JUMANPP_DIC_ENTRIES_H
#define JUMANPP_DIC_ENTRIES_H

#include <core/impl/field_reader.h>
#include <util/array_slice.h>
#include <util/string_piece.h>
#include <util/types.hpp>
#include "darts_trie.h"

namespace jumanpp {
namespace core {
namespace dic {

struct EntriesHolder {
  DoubleArray trie;
  i32 entrySize;
  impl::IntStorageReader entries;
  impl::IntStorageReader entryPtrs;
};

class IndexedEntries {
  i32 entrySize_;
  impl::IntListTraversal entries_;
  impl::IntStorageReader entryData_;
  i32 count_;
  i32 remaining_;
  i32 lastIdx = 0;

 public:
  IndexedEntries(i32 entrySize_, const impl::IntListTraversal& entries_,
                 const impl::IntStorageReader& entryData_)
      : entrySize_(entrySize_),
        entries_(entries_),
        entryData_(entryData_),
        count_(entries_.size()),
        remaining_{entries_.size()} {}

  i32 count() const { return count_; }
  i32 remaining() const { return remaining_; }

  bool fillEntryData(util::MutableArraySlice<i32>* result) {
    if (remaining_ <= 0) {
      return false;
    }
    JPP_DCHECK_EQ(entrySize_, result->size());
    JPP_RET_CHECK(entries_.readOneCumulative(&lastIdx));
    auto actualData = entryData_.rawWithLimit(lastIdx, entrySize_);
    JPP_RET_CHECK(actualData.fill(*result, result->size()) == entrySize_);
    remaining_ -= 1;
    return true;
  }
};

class IndexTraversal {
  DoubleArrayTraversal da_;
  EntriesHolder* dic_;

 public:
  IndexTraversal(EntriesHolder* dic_)
      : da_(dic_->trie.traversal()), dic_(dic_) {}
  TraverseStatus step(const StringPiece& sp) { return da_.step(sp); }
  IndexedEntries entries() const {
    auto reader = dic_->entryPtrs.listAt(da_.value());
    return IndexedEntries{dic_->entrySize, reader, dic_->entries};
  }
};

class DictionaryEntries {
  EntriesHolder* data_;

 public:
  DictionaryEntries(EntriesHolder* data_) noexcept : data_(data_) {}
  i32 entrySize() const { return static_cast<i32>(data_->entrySize); }
  IndexTraversal traversal() const { return IndexTraversal(data_); }
};

}  // dic
}  // core
}  // jumanpp

#endif  // JUMANPP_DIC_ENTRIES_H
