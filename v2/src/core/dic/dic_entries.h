//
// Created by Arseny Tolmachev on 2017/02/21.
//

#ifndef JUMANPP_DIC_ENTRIES_H
#define JUMANPP_DIC_ENTRIES_H

#include "core/core_types.h"
#include "core/dic/darts_trie.h"
#include "core_config.h"
#include "field_reader.h"
#include "util/array_slice.h"
#include "util/string_piece.h"
#include "util/types.hpp"

namespace jumanpp {
namespace core {
namespace dic {

class DicEntryBuffer {
  u32 numFeatures_ = 0;
  u32 numData_ = 0;
  i32 featureBuffer_[JPP_MAX_DIC_FIELDS];
  i32 dataBuffer_[JPP_MAX_DIC_FIELDS];
  impl::IntListTraversal remainingData_;

  friend class IndexedEntries;

 public:
  DicEntryBuffer() = default;

  util::ArraySlice<i32> features() const {
    return util::ArraySlice<i32>{featureBuffer_, numFeatures_};
  }

  util::ArraySlice<i32> data() const {
    if (remainingData_.size() == 0) {
      return util::ArraySlice<i32>{featureBuffer_ + numFeatures_, numData_};
    }

    return util::ArraySlice<i32>{dataBuffer_, numData_};
  }

  bool nextData() {
    if (remainingData_.size() == 0) {  // data is stored in features fully
      dataBuffer_[0] -= 1;
      return dataBuffer_[0] >= 0;
    }

    if (remainingData_.remaining() == 0) {
      return false;
    }

    auto res = remainingData_.fill(dataBuffer_, numData_);
    return res == numData_;
  }

  void setCounts(u32 numFeatures, u32 numData) {
    numFeatures_ = numFeatures;
    numData_ = numData;
  }

  u32 numFeatures() const { return numFeatures_; }
  u32 numData() const { return numData_; }
  i32 remaining() const {
    if (remainingData_.size() == 0) {
      return dataBuffer_[0];
    } else {
      return remainingData_.remaining() / numData_;
    }
  }

  bool fillFromStorage(EntryPtr ptr, const impl::IntStorageReader& entryData) {
    if (ptr.isAlias()) {
      auto totalCnt = numFeatures_ + 1;
      auto actualData = entryData.rawWithLimit(ptr.dicPtr(), totalCnt);
      auto num = actualData.fill(featureBuffer_, totalCnt);
      if (num < totalCnt) {
        return false;
      }
      auto numAlias = featureBuffer_[numFeatures_];
      auto dataCnt = numData_ * numAlias;
      remainingData_ = entryData.rawWithLimit(actualData.pointer(), dataCnt);
    } else {
      auto totalCnt = numFeatures_ + numData_;
      auto actualData = entryData.rawWithLimit(ptr.dicPtr(), totalCnt);
      auto num = actualData.fill(featureBuffer_, totalCnt);
      if (num < totalCnt) {
        return false;
      }
      remainingData_.clear();
      dataBuffer_[0] = 1;
    }

    return true;
  }

  void overwriteFeaturesWith(util::ArraySlice<i32> newFeatures) {
    JPP_DCHECK_LT(newFeatures.size(), JPP_MAX_DIC_FIELDS);
    for (int i = 0; i < newFeatures.size(); ++i) {
      featureBuffer_[i] = newFeatures[i];
    }
  }
};

class IndexedEntries {
  i32 numFeatures_;
  i32 numData_;
  impl::IntListTraversal entries_;
  impl::IntStorageReader entryData_;
  i32 lastIdx_ = 0;

 public:
  IndexedEntries(i32 numFeatures, i32 dataSize,
                 const impl::IntListTraversal& entries_,
                 const impl::IntStorageReader& entryData_)
      : numFeatures_(numFeatures),
        numData_{dataSize},
        entries_(entries_),
        entryData_(entryData_) {}

  i32 count() const { return entries_.size(); }
  i32 remaining() const { return entries_.remaining(); }

  inline bool readOnePtr() {
    if (entries_.empty()) {
      return false;
    }
    JPP_RET_CHECK(entries_.readOneCumulative(&lastIdx_));
    return true;
  }

  EntryPtr currentPtr() const noexcept { return EntryPtr{lastIdx_}; }

  bool fillEntryData(DicEntryBuffer* result) {
    JPP_DCHECK(entries_.didRead());
    result->setCounts(static_cast<u32>(numFeatures_),
                      static_cast<u32>(numData_));
    auto ptr = currentPtr();
    return result->fillFromStorage(ptr, entryData_);
  }
};

struct EntriesHolder {
  DoubleArray trie;
  i32 numFeatures;
  i32 numData;
  impl::IntStorageReader entries;
  impl::IntStorageReader entryPtrs;

  IndexedEntries entryTraversal(i32 ptr) const {
    auto entries = entryPtrs.listAt(ptr);
    return IndexedEntries{numFeatures, numData, entries, this->entries};
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
  i32 numFeatures() const { return static_cast<i32>(data_->numFeatures); }
  i32 numData() const { return static_cast<i32>(data_->numData); }

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
    auto rdr = data_->entries.rawWithLimit(ptr, data_->numFeatures);
    return rdr;
  }

  const impl::IntStorageReader& entryData() const { return data_->entries; }
};

}  // namespace dic
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_DIC_ENTRIES_H
