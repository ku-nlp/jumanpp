//
// Created by Arseny Tolmachev on 2017/03/06.
//

#ifndef JUMANPP_SLICEABLE_ARRAY_H
#define JUMANPP_SLICEABLE_ARRAY_H

#include "util/array_slice.h"

namespace jumanpp {
namespace util {

template <typename T>
class Sliceable {
  using underlying = util::MutableArraySlice<T>;
  underlying data_;
  size_t rowSize_;
  size_t numRows_;

 public:
  using size_type = typename util::MutableArraySlice<T>::size_type;
  using value_type = typename util::MutableArraySlice<T>::value_type;

  constexpr Sliceable(const MutableArraySlice<T>& data_, size_t rowSize_,
                      size_t numRows_)
      : data_(data_), rowSize_(rowSize_), numRows_{numRows_} {
    JPP_DCHECK_EQ(data_.size(), rowSize_ * numRows_);
  }

  T& operator[](size_type idx) { return data_.at(idx); }
  const T& operator[](size_type idx) const { return data_.at(idx); }
  T& at(size_type idx) { return data_.at(idx); }
  const T& at(size_type idx) const { return data_.at(idx); }
  util::MutableArraySlice<T> row(i32 i) {
    JPP_DCHECK_IN(i, 0, numRows_);
    return util::MutableArraySlice<T>{data_, rowSize_ * i, rowSize_};
  }
  util::ArraySlice<T> row(i32 i) const {
    JPP_DCHECK_IN(i, 0, numRows_);
    return util::ArraySlice<T>{data_, rowSize_ * i, rowSize_};
  }

  util::MutableArraySlice<T> data() { return data_; }

  util::ArraySlice<T> data() const { return util::ArraySlice<T>{data_}; }

  Sliceable<T> topRows(size_t nrows) const {
    JPP_DCHECK_GE(nrows, 0);
    JPP_DCHECK_LE(nrows, numRows_);
    underlying slice{data_, 0, rowSize_ * nrows};
    return Sliceable<T>{slice, rowSize_, nrows};
  }

  Sliceable<T> rows(size_t start, size_t end) {
    JPP_DCHECK_IN(start, 0, numRows_);
    JPP_DCHECK_LE(start, end);
    JPP_DCHECK_LE(end, numRows_);
    auto numElem = end - start;
    underlying slice{data_, start * rowSize_, rowSize_ * numElem};
    return Sliceable<T>{slice, rowSize_, numElem};
  }

  size_t size() const { return data_.size(); }
  size_t rowSize() const { return rowSize_; }
  size_t numRows() const { return numRows_; }

  typename underlying::iterator begin() { return data_.begin(); }
  typename underlying::const_iterator begin() const { return data_.begin(); }
  typename underlying::iterator end() { return data_.end(); }
  typename underlying::const_iterator end() const { return data_.end(); }
};

}  // namespace util
}  // namespace jumanpp

#endif  // JUMANPP_SLICEABLE_ARRAY_H
