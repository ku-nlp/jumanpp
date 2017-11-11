//
// Created by Arseny Tolmachev on 2017/11/11.
//

#ifndef JUMANPP_QUANTIZED_WEIGHTS_H
#define JUMANPP_QUANTIZED_WEIGHTS_H

#include "util/common.hpp"

namespace numanpp {
namespace util {

class Float8BitLinearQ {
  char* memory_;
  size_t size_;
  float min_;
  float step_;

 public:
  Float8BitLinearQ(char* memory, size_t size, float min, float step)
      : memory_(memory), size_(size), min_(min), step_(step) {}
  size_t size() const { return size_; }
  float at(size_t idx) {
    JPP_DCHECK_IN(idx, 0, size_);
    unsigned char data = static_cast<unsigned char>(memory_[idx]);
    return min_ + step_ * data;
  }
};

}  // namespace util
}  // namespace numanpp

#endif  // JUMANPP_QUANTIZED_WEIGHTS_H
