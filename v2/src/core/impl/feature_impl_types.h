//
// Created by Arseny Tolmachev on 2017/03/03.
//

#ifndef JUMANPP_FEATURE_IMPL_TYPES_H
#define JUMANPP_FEATURE_IMPL_TYPES_H

#include "core/core_types.h"
#include "util/array_slice.h"

namespace jumanpp {
namespace core {
namespace features {
namespace impl {

class PrimitiveFeatureData {
  util::ArraySlice<EntryPtr> entries_;
  util::ArraySlice<i32> entryData_;
  util::MutableArraySlice<u64> features_;
  i32 index_ = -1;
  size_t entrySize_;
  size_t featureCnt_;

 public:
  PrimitiveFeatureData(const util::ArraySlice<EntryPtr>& entries_,
                       const util::ArraySlice<i32>& entryData_,
                       const util::MutableArraySlice<u64>& features_)
      : entries_(entries_),
        entryData_(entryData_),
        features_(features_),
        entrySize_{entryData_.size() / entries_.size()},
        featureCnt_{features_.size() / entries_.size()} {}

  bool next() {
    ++index_;
    return index_ < entries_.size();
  }

  EntryPtr entry() const { return entries_[index_]; }

  util::ArraySlice<i32> entryData() const {
    return util::ArraySlice<i32>{entryData_, index_ * entrySize_, entrySize_};
  }

  util::MutableArraySlice<u64> featureData() const {
    return util::MutableArraySlice<u64>{features_, index_ * featureCnt_,
                                        featureCnt_};
  }
};

}  // impl
}  // features
}  // core
}  // jumanpp

#endif  // JUMANPP_FEATURE_IMPL_TYPES_H
