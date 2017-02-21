//
// Created by Arseny Tolmachev on 2017/02/20.
//

#ifndef JUMANPP_DIC_READER_H
#define JUMANPP_DIC_READER_H

#include <util/coded_io.h>

namespace jumanpp {
namespace core {
namespace dic {
namespace impl {

class StringStorageReader {
  StringPiece data_;

 public:
  StringStorageReader(const StringPiece& obj) noexcept : data_{obj} {}

  StringPiece data() const noexcept { return data_; }

  bool readAt(i32 ptr, StringPiece* ret) const noexcept {
    util::CodedBufferParser parser;
    parser.reset(data_.slice(ptr, data_.size()));
    return parser.readStringPiece(ret);
  }
};

class IntListTraversal {
  i32 length_;
  i32 position_ = 0;
  util::CodedBufferParser parser_;

 public:
  IntListTraversal(i32 length, const util::CodedBufferParser& parser)
      : length_{length}, parser_{parser} {}

  i32 size() const noexcept { return length_; }
  i32 remaining() const noexcept { return length_ - position_; }

  bool readOne(i32* result) noexcept {
    bool ret = parser_.readInt(result) & (position_ < length_);
    position_ += 1;
    return ret;
  }

  bool readOneCumulative(i32* result) noexcept {
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
};

class IntStorageReader {
  StringPiece data_;

 public:
  IntStorageReader() {}
  IntStorageReader(const StringPiece& obj) : data_{obj} {}

  IntListTraversal rawWithLimit(i32 ptr, i32 length) {
    JPP_DCHECK_GE(ptr, 0);
    JPP_DCHECK_LT(ptr, data_.size());
    JPP_DCHECK_GE(length, 0);
    JPP_DCHECK_LE(length, data_.size() - ptr);  // this is a lower bound
    util::CodedBufferParser parser{data_.from(ptr)};
    return IntListTraversal{length, parser};
  }

  IntListTraversal listAt(i32 ptr) const noexcept {
    JPP_DCHECK_GE(ptr, 0);
    JPP_DCHECK_LT(ptr, data_.size());
    util::CodedBufferParser parser{data_.from(ptr)};
    i32 size;
    if (!parser.readInt(&size)) {
      // empty traversal
      parser.limit(0);
      return IntListTraversal{-1, parser};
    }

    return IntListTraversal{size, parser};
  }
};

class StringStorageTraversal {
  util::CodedBufferParser parser_;
  i32 position_ = 0;

 public:
  StringStorageTraversal(const StringPiece& data) : parser_{data} {}

  bool hasNext() const noexcept { return parser_.remaining() != 0; }

  i32 position() const noexcept { return position_; }

  bool next(StringPiece* result) noexcept {
    position_ = parser_.position();
    bool ret = parser_.readStringPiece(result);
    return ret;
  }
};

}  // impl
}  // dic
}  // core
}  // jumanpp

#endif  // JUMANPP_DIC_READER_H
