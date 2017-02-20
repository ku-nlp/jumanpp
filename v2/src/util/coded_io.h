//
// Created by Arseny Tolmachev on 2017/02/19.
//

#ifndef JUMANPP_CODED_IO_H
#define JUMANPP_CODED_IO_H

#include <vector>
#include "string_piece.h"
#include "types.hpp"

namespace jumanpp {
namespace util {

// Some logic of this file is borrowed from Google Protocol Buffer project,
// which is BSD-3 licensed.
// https://github.com/google/protobuf

namespace impl {

constexpr size_t UInt64MaxSize = 10;

inline u8* writeVarint64ToArray(u64 value, u8* target) {
  while (value >= 0x80) {
    *target = static_cast<u8>(value | 0x80);
    value >>= 7;
    ++target;
  }
  *target = static_cast<u8>(value);
  return target + 1;
}

inline u8* writeRawBytes(const u8* begin, const u8* end, u8* target) {
  std::copy(begin, end, target);
  return target + std::distance(begin, end);
}

inline u8* writeRawBytes(StringPiece::iterator begin, StringPiece::iterator end,
                         u8* target) {
  std::copy(begin, end, target);
  return target + std::distance(begin, end);
}
}  // impl

class CodedBuffer {
  std::vector<u8> buffer_;
  u8* front_ = nullptr;
  u8* end_ = nullptr;

  inline size_t remaining() const noexcept { return end_ - front_; }

  void grow(size_t atLeast);

 public:
  size_t position() const noexcept {
    return static_cast<size_t>(front_ - buffer_.data());
  }

  void reset() { front_ = buffer_.data(); }

  void writeVarint(u64 value) {
    if (JPP_UNLIKELY(remaining() < impl::UInt64MaxSize)) {
      grow(10);
    }
    front_ = impl::writeVarint64ToArray(value, front_);
  }

  void writeString(StringPiece value) {
    auto vsize = value.size();
    size_t maxSize = vsize + impl::UInt64MaxSize;
    if (JPP_UNLIKELY(remaining() < maxSize)) {
      grow(maxSize);
    }
    front_ = impl::writeVarint64ToArray(vsize, front_);
    impl::writeRawBytes(value.begin(), value.end(), front_);
    front_ += vsize;
  }

  StringPiece contents() const noexcept {
    auto begin = reinterpret_cast<StringPiece::iterator>(buffer_.data());
    auto end = reinterpret_cast<StringPiece::iterator>(front_);
    return StringPiece{begin, end};
  }
};

#define JPP_RET_CHECK(x) { if(!JPP_LIKELY(x)) return false; }

class CodedBufferParser {
  const u8* position_ = nullptr;
  const u8* end_ = nullptr;
  const u8* begin_ = nullptr;

 public:
  CodedBufferParser() {}

  CodedBufferParser(StringPiece data) { reset(data); }

  void reset(StringPiece data) {
    position_ = reinterpret_cast<const u8*>(data.begin());
    begin_ = position_;
    end_ = reinterpret_cast<const u8*>(data.end());
  }

  bool readVarint64Slowpath(u64* pInt) noexcept;

  // most of varints are small
  inline bool readVarint64(u64* ptr) noexcept {
    if (JPP_LIKELY(position_ < end_) && *position_ < 0x80) {
      *ptr = *position_;
      ++position_;
      return true;
    }
    return readVarint64Slowpath(ptr);
  }

  bool readStringPiece(StringPiece* ptr) noexcept {
    size_t len;
    if (!readInt(&len)) return false;
    if (len > remaining()) {
      return false;
    }
    auto begin = reinterpret_cast<StringPiece::iterator>(position_);
    position_ += len;
    auto end = reinterpret_cast<StringPiece::iterator>(position_);
    *ptr = StringPiece{begin, end};
    return true;
  }

  template <typename T>
  inline bool readInt(T* ptr) noexcept {
    u64 mem;
    JPP_RET_CHECK(readVarint64(&mem));
    *ptr = static_cast<T>(mem);
    return true;
  }

  size_t remaining() const noexcept {
    return static_cast<size_t>(end_ - position_);
  };

  i32 position() const noexcept {
    return static_cast<i32>(position_ - begin_);
  };

  size_t limit(size_t cnt) {
    size_t cur = remaining();
    end_ = std::min(end_, position_ + cnt);
    return cur;
  }
};

}  // util
}  // jumanpp

#endif  // JUMANPP_CODED_IO_H
