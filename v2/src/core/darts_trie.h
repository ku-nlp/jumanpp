//
// Created by Arseny Tolmachev on 2017/02/18.
//

#ifndef JUMANPP_DARTS_TRIE_H
#define JUMANPP_DARTS_TRIE_H


#include <cstring>
#include <memory>
#include <util/types.hpp>
#include <util/string_piece.h>
#include <util/status.hpp>

namespace Darts {
template <class node_type_, class node_u_type_, class array_type_,
    class array_u_type_, class length_func_>
class DoubleArrayImpl;
}


namespace jumanpp {
namespace core {

struct CharStringLength {
  size_t operator()(const char* data) const { return std::strlen(data); }
};


namespace impl {

using DoubleArray = Darts::DoubleArrayImpl<char, unsigned char, i32, u32, CharStringLength>;

struct PieceWithValue {
  StringPiece key;
  int value;
  PieceWithValue(const StringPiece &key, int value) : key(key), value(value) {}
};

}


class DoubleArrayBuilder {
  std::unique_ptr<impl::DoubleArray> array_;
  std::vector<impl::PieceWithValue> immediate_;

public:
  DoubleArrayBuilder();
  void add(StringPiece key, int value);
  Status build();
  size_t underlyingByteSize() const;
  const void* underlyingStorage() const;

  ~DoubleArrayBuilder();
};



}
}

#endif  // JUMANPP_DARTS_TRIE_H
