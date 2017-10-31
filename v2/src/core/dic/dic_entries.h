//
// Created by Arseny Tolmachev on 2017/02/21.
//

#ifndef JUMANPP_DIC_ENTRIES_H
#define JUMANPP_DIC_ENTRIES_H

#include "core/core_types.h"
#include "core/dic/darts_trie.h"
#include "field_reader.h"
#include "util/array_slice.h"
#include "util/string_piece.h"
#include "util/types.hpp"

namespace jumanpp {
namespace core {
namespace dic {

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

  inline bool readOnePtr(i32* result) {
    if (remaining_ <= 0) {
      return false;
    }
    JPP_RET_CHECK(entries_.readOneCumulative(&lastIdx));
    *result = lastIdx;
    return true;
  }

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

struct EntriesHolder {
  DoubleArray trie;
  i32 entrySize;
  impl::IntStorageReader entries;
  impl::IntStorageReader entryPtrs;

  IndexedEntries entryTraversal(i32 ptr) const {
    auto entries = entryPtrs.listAt(ptr);
    return IndexedEntries{entrySize, entries, this->entries};
  }
};

class IndexTraversal {
  DoubleArrayTraversal da_;
  const EntriesHolder* dic_;

 public:
  explicit IndexTraversal(const EntriesHolder* dic_)
      : da_(dic_->trie.traversal()), dic_(dic_) {}
  TraverseStatus step(const StringPiece& sp) { return da_.step(sp); }
  TraverseStatus step(const StringPiece& sp, size_t& pos) {
    return da_.step(sp, pos);
  }
  IndexedEntries entries() const { return dic_->entryTraversal(da_.value()); }
};

class DictionaryEntries {
  const EntriesHolder* data_;

 public:
  explicit DictionaryEntries(const EntriesHolder* data_) noexcept
      : data_(data_) {}
  i32 entrySize() const { return static_cast<i32>(data_->entrySize); }
  IndexTraversal traversal() const { return IndexTraversal(data_); }
  DoubleArrayTraversal doubleArrayTraversal() const {
    return data_->trie.traversal();
  }

  IndexedEntries entryTraversal(const DoubleArrayTraversal& at) const {
    return data_->entryTraversal(at.value());
  }

  impl::IntListTraversal entryAtPtr(EntryPtr ptr) const {
    return entryAtPtr(ptr.dicPtr());
  }
  impl::IntListTraversal entryAtPtr(i32 ptr) const {
    auto rdr = data_->entries.rawWithLimit(ptr, data_->entrySize);
    return rdr;
  }
};

}  // namespace dic
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_DIC_ENTRIES_H
