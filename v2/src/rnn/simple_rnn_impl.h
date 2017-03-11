//
// Created by Arseny Tolmachev on 2017/03/10.
//

#ifndef JUMANPP_SIMPLE_RNN_IMPL_H
#define JUMANPP_SIMPLE_RNN_IMPL_H

#include <eigen3/Eigen/Core>
#include "util/memory.hpp"
#include "util/sliceable_array.h"

namespace jumanpp {
namespace rnn {
namespace impl {

using FpType = float;

using MatrixType = Eigen::Matrix<FpType, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>;
using Matrix = Eigen::Map<MatrixType, Eigen::Aligned64>;

template <typename T>
inline bool isAligned(const T* ptr, size_t align) {
  size_t data_sz = reinterpret_cast<size_t>(ptr);
  return util::memory::IsAligned(data_sz, align);
}

inline Matrix asMatrix(const util::ArraySlice<FpType> &slice, size_t rows, size_t cols) {
  FpType* ptr = const_cast<FpType *>(slice.begin());
  JPP_DCHECK_LE(cols*rows, slice.size());
  JPP_DCHECK(isAligned(ptr, 64));
  return Matrix(ptr, rows, cols);
}

inline Matrix asMatrix(const util::MutableArraySlice<FpType> &slice, size_t rows, size_t cols) {
  FpType* ptr = slice.begin();
  JPP_DCHECK_LE(cols*rows, slice.size());
  JPP_DCHECK(isAligned(ptr, 64));
  return Matrix(ptr, rows, cols);
}

inline Matrix asMatrix(const util::Sliceable<FpType> &sl, size_t rows, size_t cols) {
  FpType* ptr = const_cast<FpType *>(sl.data().data());
  JPP_DCHECK_LE(cols*rows, sl.size());
  JPP_DCHECK(isAligned(ptr, 64));
  return Matrix(ptr, rows, cols);
}

} // impl
} // rnn
} // jumanpp

#endif //JUMANPP_SIMPLE_RNN_IMPL_H
