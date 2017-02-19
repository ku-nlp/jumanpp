//
// Created by Arseny Tolmachev on 2017/02/19.
//

#include "coded_io.h"
#include <utility>
#include "common.hpp"
#include "types.hpp"

namespace jumanpp {
namespace util {

void jumanpp::util::CodedBuffer::grow(size_t atLeast) {
  auto offset = front_ - buffer_.data();
  auto curSize = buffer_.size();
  auto autosize = curSize * 3 / 2;
  auto newsize = curSize + std::max(autosize - curSize, atLeast);
  buffer_.resize(newsize);
  front_ = buffer_.data() + offset;
  end_ = buffer_.data() + newsize;
}

namespace impl {

// Read a varint from the given buffer, write it to *value, and return a pair.
// The first part of the pair is true iff the read was successful.  The second
// part is buffer + (number of bytes read).  This function is always inlined,
// so returning a pair is costless.
// Adopted from
// https://github.com/google/protobuf/blob/master/src/google/protobuf/io/coded_stream.cc
JPP_ALWAYS_INLINE::std::pair<bool, const u8*> ReadVarint64FromArray(
    const u8* buffer, u64* value);
inline ::std::pair<bool, const u8*> ReadVarint64FromArray(const u8* buffer,
                                                          u64* value) {
  const u8* ptr = buffer;
  u32 b;

  // Splitting into 32-bit pieces gives better performance on 32-bit
  // processors.
  u32 part0 = 0, part1 = 0, part2 = 0;

  b = *(ptr++);
  part0 = b;
  if (!(b & 0x80)) goto done;
  part0 -= 0x80;
  b = *(ptr++);
  part0 += b << 7;
  if (!(b & 0x80)) goto done;
  part0 -= 0x80 << 7;
  b = *(ptr++);
  part0 += b << 14;
  if (!(b & 0x80)) goto done;
  part0 -= 0x80 << 14;
  b = *(ptr++);
  part0 += b << 21;
  if (!(b & 0x80)) goto done;
  part0 -= 0x80 << 21;
  b = *(ptr++);
  part1 = b;
  if (!(b & 0x80)) goto done;
  part1 -= 0x80;
  b = *(ptr++);
  part1 += b << 7;
  if (!(b & 0x80)) goto done;
  part1 -= 0x80 << 7;
  b = *(ptr++);
  part1 += b << 14;
  if (!(b & 0x80)) goto done;
  part1 -= 0x80 << 14;
  b = *(ptr++);
  part1 += b << 21;
  if (!(b & 0x80)) goto done;
  part1 -= 0x80 << 21;
  b = *(ptr++);
  part2 = b;
  if (!(b & 0x80)) goto done;
  part2 -= 0x80;
  b = *(ptr++);
  part2 += b << 7;
  if (!(b & 0x80)) goto done;
  // "part2 -= 0x80 << 7" is irrelevant because (0x80 << 7) << 56 is 0.

  // We have overrun the maximum size of a varint (10 bytes).  Assume
  // the data is corrupt.
  return std::make_pair(false, ptr);

done:
  *value = (static_cast<u64>(part0)) | (static_cast<u64>(part1) << 28) |
           (static_cast<u64>(part2) << 56);
  return std::make_pair(true, ptr);
}
}  // impl

bool CodedBufferParser::readVarint64Slowpath(u64* pInt) {
  if (remaining() == 0) return false;

  // We won't get to the end of buffer with this decode
  // So we can use faster implementation.
  if (remaining() >= impl::UInt64MaxSize) {
    auto ret = impl::ReadVarint64FromArray(position_, pInt);
    if (ret.first) {
      position_ = ret.second;
    }
    return ret.first;
  }

  // Slowest implementation: cycle.
  u64 result = 0;
  int count = 0;
  u32 b;
  do {
    if (count == impl::UInt64MaxSize) {
      *pInt = 0;
      return false;
    }

    b = *position_;
    result |= static_cast<u64>(b & 0x7F) << (7 * count);
    ++position_;
    ++count;
  } while (b & 0x80);

  *pInt = result;
  return true;
}

}  // jumanpp
}  // util
