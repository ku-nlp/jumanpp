//
// Created by Arseny Tolmachev on 2017/02/18.
//

#ifndef JUMANPP_DARTS_TRIE_H
#define JUMANPP_DARTS_TRIE_H

#include "util/string_piece.h"
#include <memory>
#include "util/status.hpp"
#include "util/types.hpp"
#include <vector>

namespace Darts {
template <class node_type_, class node_u_type_, class array_type_,
          class array_u_type_, class length_func_>
class DoubleArrayImpl;
}

namespace jumanpp {
namespace core {

struct CharStringLength {
  size_t operator()(StringPiece::pointer_t data) const {
    JPP_DCHECK_NOT("should never be called");
    return 0;
  }
};

namespace impl {

using DoubleArrayCore =
    Darts::DoubleArrayImpl<u8, u8, i32, u32, CharStringLength>;

struct PieceWithValue {
  StringPiece key;
  int value;
  PieceWithValue(const StringPiece& key, int value) : key(key), value(value) {}
};
}

class DoubleArrayBuilder {
  std::unique_ptr<impl::DoubleArrayCore> array_;
  std::vector<impl::PieceWithValue> immediate_;

 public:
  DoubleArrayBuilder();
  void add(StringPiece key, int value);
  Status build();
  size_t underlyingByteSize() const;
  const void* underlyingStorage() const;

  StringPiece result() const {
    return StringPiece{
        reinterpret_cast<StringPiece::pointer_t>(underlyingStorage()),
        underlyingByteSize()};
  }

  ~DoubleArrayBuilder();
};

class DoubleArray;

enum class TraverseStatus { Ok, NoLeaf, NoNode };

class DoubleArrayTraversal {
  const impl::DoubleArrayCore* base_;
  size_t node_pos_ = 0;
  size_t key_pos_ = 0;
  i32 value_;

 public:
  DoubleArrayTraversal(const impl::DoubleArrayCore* base_) noexcept
      : base_(base_) {}
  DoubleArrayTraversal(const DoubleArrayTraversal&) noexcept = default;
  i32 value() const { return value_; }
  TraverseStatus step(StringPiece data);
};

namespace impl {
struct DoubleArrayBackingFile;
constexpr size_t InvalidSize = ~size_t(0);
}

class DoubleArray {
  std::unique_ptr<impl::DoubleArrayCore> underlying_;
  std::unique_ptr<impl::DoubleArrayBackingFile> file_;

 public:
  Status loadFromMemory(StringPiece memory);
  Status loadFromFile(StringPiece filename, size_t offset = 0,
                      size_t length = impl::InvalidSize);
  DoubleArray();
  ~DoubleArray();

  DoubleArrayTraversal traversal() const {
    return DoubleArrayTraversal(underlying_.get());
  }

  friend class DoubleArrayTraversal;
};
}
}

#endif  // JUMANPP_DARTS_TRIE_H
