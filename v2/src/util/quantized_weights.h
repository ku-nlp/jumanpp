//
// Created by Arseny Tolmachev on 2017/11/11.
//

#ifndef JUMANPP_QUANTIZED_WEIGHTS_H
#define JUMANPP_QUANTIZED_WEIGHTS_H

#include "util/common.hpp"

namespace jumanpp {
namespace util {

class Float8BitLinearQ {
  const unsigned char* memory_;
  size_t size_;
  float min_;
  float step_;

 public:
  Float8BitLinearQ(const char* memory, size_t size, float min, float step)
      : memory_(reinterpret_cast<const unsigned char*>(memory)), size_(size), min_(min), step_(step) {}
  size_t size() const { return size_; }
  float at(size_t idx) const {
    JPP_DCHECK_IN(idx, 0, size_);
    unsigned char data = memory_[idx];
    return min_ + step_ * data;
  }
  template<util::PrefetchHint kind>
  void prefetch(size_t idx) const {}
};

}  // namespace util
}  // namespace jumanpp

#endif  // JUMANPP_QUANTIZED_WEIGHTS_H
