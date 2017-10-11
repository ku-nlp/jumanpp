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

}  // namespace impl

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
      grow(impl::UInt64MaxSize);
    }
    front_ = impl::writeVarint64ToArray(value, front_);
  }

  void writeFixed32(u32 value) {
    if (JPP_UNLIKELY(remaining() < sizeof(u32))) {
      grow(sizeof(u32));
    }
    *(front_ + 0) = static_cast<u8>((value >> 0) & 0xff);
    *(front_ + 1) = static_cast<u8>((value >> 8) & 0xff);
    *(front_ + 2) = static_cast<u8>((value >> 16) & 0xff);
    *(front_ + 3) = static_cast<u8>((value >> 24) & 0xff);
    front_ += 4;
  }

  void writeStringDataWithoutLengthPrefix(StringPiece value) {
    auto vsize = value.size();
    if (JPP_UNLIKELY(remaining() < vsize + impl::UInt64MaxSize)) {
      grow(vsize + impl::UInt64MaxSize);
    }
    impl::writeRawBytes(value.ubegin(), value.uend(), front_);
    front_ += vsize;
  }

  void writeString(StringPiece value) {
    auto vsize = value.size();
    size_t maxSize = vsize + impl::UInt64MaxSize;
    if (JPP_UNLIKELY(remaining() < maxSize)) {
      grow(maxSize);
    }
    front_ = impl::writeVarint64ToArray(vsize, front_);
    impl::writeRawBytes(value.ubegin(), value.uend(), front_);
    front_ += vsize;
  }

  StringPiece contents() const noexcept {
    auto begin = reinterpret_cast<StringPiece::iterator>(buffer_.data());
    auto end = reinterpret_cast<StringPiece::iterator>(front_);
    return StringPiece{begin, end};
  }
};

class CodedBufferParser {
  const u8* position_ = nullptr;
  const u8* end_ = nullptr;
  const u8* begin_ = nullptr;

 public:
  CodedBufferParser() = default;

  explicit CodedBufferParser(StringPiece data) noexcept { reset(data); }

  void reset(StringPiece data) noexcept {
    position_ = data.ubegin();
    begin_ = position_;
    end_ = data.uend();
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

  inline bool readFixed32(u32* result) {
    if (remaining() < sizeof(u32)) {
      return false;
    }
    u32 data = 0;
    data |= static_cast<u32>(*(position_ + 0)) << 0;
    data |= static_cast<u32>(*(position_ + 1)) << 8;
    data |= static_cast<u32>(*(position_ + 2)) << 16;
    data |= static_cast<u32>(*(position_ + 3)) << 24;
    *result = data;
    position_ += 4;
    return true;
  }

  size_t remaining() const noexcept {
    return static_cast<size_t>(end_ - position_);
  };

  i32 position() const noexcept {
    return static_cast<i32>(position_ - begin_);
  };

  size_t limit(size_t cnt) noexcept {
    size_t cur = remaining();
    end_ = std::min(end_, position_ + cnt);
    return cur;
  }
};

}  // namespace util
}  // namespace jumanpp

#endif  // JUMANPP_CODED_IO_H
