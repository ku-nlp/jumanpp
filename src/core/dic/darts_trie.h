//
// Created by Arseny Tolmachev on 2017/02/18.
//

#ifndef JUMANPP_DARTS_TRIE_H
#define JUMANPP_DARTS_TRIE_H

#include <memory>
#include <vector>
#include "util/status.hpp"
#include "util/string_piece.h"
#include "util/types.hpp"

namespace JppDarts {
template <typename, typename, typename T, typename>
class DoubleArrayImpl;
}

namespace jumanpp {
namespace core {
class ProgressCallback;
namespace dic {
namespace impl {

using DoubleArrayCore =
    JppDarts::DoubleArrayImpl<char, unsigned char, i32, u32>;

struct PieceWithValue {
  StringPiece key;
  i32 value;

  PieceWithValue(StringPiece key, i32 value) : key(key), value(value) {}
};

}  // namespace impl

class DoubleArray;

class DoubleArrayBuilder {
  std::unique_ptr<impl::DoubleArrayCore> array_;
  std::vector<impl::PieceWithValue> immediate_;

 public:
  DoubleArrayBuilder();
  void add(StringPiece key, int value);
  Status build(ProgressCallback *progress = nullptr);
  size_t underlyingByteSize() const;
  const void *underlyingStorage() const;

  StringPiece result() const {
    auto storage =
        reinterpret_cast<StringPiece::pointer_t>(underlyingStorage());
    return StringPiece{storage, underlyingByteSize()};
  }

  friend class DoubleArray;

  ~DoubleArrayBuilder();
};

enum class TraverseStatus {
  Ok,      // There was a leaf node in the trie
  NoLeaf,  // There wasn't a leaf node, but there may exist one
           // when continuing searching
  NoNode   // Even if continue searching there would be no nodes in the trie
};

class DoubleArrayTraversal {
  const impl::DoubleArrayCore *base_;
  size_t node_pos_ = 0;
  size_t key_pos_ = 0;
  i32 value_;

 public:
  DoubleArrayTraversal(const impl::DoubleArrayCore *base_) noexcept
      : base_(base_) {}

  DoubleArrayTraversal(const DoubleArrayTraversal &) noexcept = default;

  i32 value() const { return value_; }
  TraverseStatus step(StringPiece data);

  bool operator==(const DoubleArrayTraversal &o) const {
    return base_ == o.base_ && node_pos_ == o.node_pos_ &&
           key_pos_ == o.key_pos_ && value_ == o.value_;
  }
};

class DoubleArray {
  std::unique_ptr<impl::DoubleArrayCore> underlying_;

 public:
  Status loadFromMemory(StringPiece memory);
  void plunder(DoubleArrayBuilder *bldr);

  DoubleArray();
  ~DoubleArray();

  DoubleArrayTraversal traversal() const {
    return DoubleArrayTraversal(underlying_.get());
  }

  StringPiece contents() const;
  std::string describe() const;

  friend class DoubleArrayTraversal;
};

}  // namespace dic
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_DARTS_TRIE_H
