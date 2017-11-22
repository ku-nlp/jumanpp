//
// Created by Arseny Tolmachev on 2017/02/20.
//

#ifndef JUMANPP_FIELD_READER_H
#define JUMANPP_FIELD_READER_H

#include "util/characters.h"
#include "util/coded_io.h"

namespace jumanpp {
namespace core {
namespace dic {
namespace impl {

class StringStorageReader {
  StringPiece data_;
  u32 alignPower_;

 public:
  explicit StringStorageReader(const StringPiece& obj, u32 alignPower) noexcept
      : data_{obj}, alignPower_{alignPower} {}

  StringPiece data() const noexcept { return data_; }

  bool readAt(i32 ptr, StringPiece* ret) const noexcept {
    i64 realPtr = static_cast<i64>(ptr) << alignPower_;
    JPP_DCHECK_IN(realPtr, 0, data_.size());
    util::CodedBufferParser parser{data_.from(realPtr)};
    return parser.readStringPiece(ret);
  }

  i32 lengthOf(i32 ptr) {
    i64 realPtr = static_cast<i64>(ptr) << alignPower_;
    JPP_DCHECK_IN(realPtr, 0, data_.size());
    util::CodedBufferParser parser{data_.from(realPtr)};
    i32 value = -1;
    parser.readInt(&value);
    return value;
  }

  i32 numCodepoints(i32 ptr) {
    i64 realPtr = static_cast<i64>(ptr) << alignPower_;
    JPP_DCHECK_IN(realPtr, 0, data_.size());
    auto start = data_.from(realPtr);
    util::CodedBufferParser parser{start};
    StringPiece result;
    if (parser.readStringPiece(&result)) {
      return chars::numCodepoints(result);
    }
    return -1;
  }

  u32 alignPower() const { return alignPower_; }
};

class IntListTraversal {
  i32 length_ = 0;
  i32 position_ = 0;
  util::CodedBufferParser parser_;

 public:
  IntListTraversal() : parser_{EMPTY_SP} {}
  IntListTraversal(i32 length, const util::CodedBufferParser& parser)
      : length_{length}, parser_{parser} {}

  i32 size() const noexcept { return length_; }
  i32 remaining() const noexcept { return length_ - position_; }
  bool empty() const noexcept { return position_ >= length_; }
  bool didRead() const noexcept { return position_ >= 0; }

  inline bool readOne(i32* result) noexcept {
    bool ret = parser_.readInt(result) & (position_ < length_);
    position_ += 1;
    return ret;
  }

  inline bool readOneCumulative(i32* result) noexcept {
    i32 diff;
    if (readOne(&diff)) {
      *result += diff;
      return true;
    }
    return false;
  }

  template <typename C>
  void fill(C& c) {
    i32 var;
    while (readOne(&var)) {
      c.push_back(var);
    }
  }

  template <typename C>
  size_t fill(C& c, size_t cnt) noexcept {
    i32 var;
    size_t nread = 0;
    for (; nread < cnt && readOne(&var); ++nread) {
      c[nread] = var;
    }
    return nread;
  }

  i32 numReadBytes() const noexcept { return parser_.numReadBytes(); }

  void clear() {
    length_ = 0;
    position_ = 0;
  }
};

class KeyValueListTraversal {
  i32 length_;
  i32 position_ = 0;
  util::CodedBufferParser parser_;

  i32 lastKey_ = 0;

  i32 key_ = 0;
  i32 value_ = 0;
  bool hasValue_ = false;

 public:
  KeyValueListTraversal(i32 length,
                        const util::CodedBufferParser& parser) noexcept
      : length_{length}, parser_{parser} {}

  inline bool moveNext() noexcept {
    JPP_RET_CHECK(hasNext());
    u64 data;
    JPP_RET_CHECK(parser_.readVarint64(&data));
    position_ += 1;
    key_ = lastKey_ + static_cast<i32>(data >> 1);
    lastKey_ = key_;
    hasValue_ = (data & 0x1) != 0;

    // we've read the key and there is no value
    if (!hasValue_) {
      return true;
    }

    JPP_RET_CHECK(parser_.readVarint64(&data));
    value_ = static_cast<i32>(data);
    return true;
  }

  inline i32 key() const noexcept { return key_; }
  inline i32 value() const noexcept { return value_; }
  inline bool hasValue() const noexcept { return hasValue_; }
  inline bool hasNext() const noexcept { return position_ < length_; }
};

class IntStorageReader {
  StringPiece data_;

 public:
  IntStorageReader() noexcept = default;
  explicit IntStorageReader(const StringPiece& obj) noexcept : data_{obj} {}

  IntListTraversal raw() const { return rawWithLimit(0, data_.size()); }

  IntListTraversal rawWithLimit(i32 ptr, i32 length) const {
    JPP_DCHECK_IN(ptr, 0, data_.size());
    JPP_DCHECK_GE(length, 0);
    JPP_DCHECK_LE(length, data_.size() - ptr);  // this is a lower bound
    util::CodedBufferParser parser{data_.from(ptr)};
    return IntListTraversal{length, parser};
  }

  IntListTraversal listAt(i32 ptr) const noexcept {
    JPP_DCHECK_IN(ptr, 0, data_.size());
    util::CodedBufferParser parser{data_.from(ptr)};
    i32 size;
    if (!parser.readInt(&size)) {
      // empty traversal
      parser.limit(0);
      return IntListTraversal{-1, parser};
    }

    return IntListTraversal{size, parser};
  }

  KeyValueListTraversal kvListAt(i32 ptr) const noexcept {
    JPP_DCHECK_IN(ptr, 0, data_.size());
    util::CodedBufferParser parser{data_.from(ptr)};
    i32 size;
    if (!parser.readInt(&size)) {
      // empty traversal
      parser.limit(0);
      return KeyValueListTraversal{-1, parser};
    }

    return KeyValueListTraversal{size, parser};
  }

  i32 lengthOf(i32 ptr) const {
    JPP_DCHECK_IN(ptr, 0, data_.size());
    util::CodedBufferParser parser{data_.from(ptr)};
    i32 value = -1;
    parser.readInt(&value);
    return value;
  }

  void prefetch(i32 ptr) const {
    util::prefetch<util::PrefetchHint::PREFETCH_HINT_T0>(data_.data() + ptr);
  }
};

class StringStorageTraversal {
  util::CodedBufferParser parser_;
  i32 lastPosition_ = 0;
  u32 alignPower_;
  u32 alignment_;

 public:
  explicit StringStorageTraversal(const StringPiece& data,
                                  u32 alignPower) noexcept
      : parser_{data}, alignPower_{alignPower}, alignment_{1u << alignPower} {}
  explicit StringStorageTraversal(const StringStorageReader& rdr) noexcept
      : StringStorageTraversal(rdr.data(), rdr.alignPower()) {}

  bool hasNext() const noexcept { return !parser_.atEnd(); }

  i32 position() const noexcept { return lastPosition_; }

  bool next(StringPiece* result) noexcept {
    lastPosition_ = static_cast<i32>(parser_.numReadBytes() >> alignPower_);
    bool ret = parser_.readStringPiece(result);
    if (JPP_LIKELY(ret)) {
      parser_.alignPointer(alignment_);
    }
    return ret;
  }
};

}  // namespace impl
}  // namespace dic
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_FIELD_READER_H
