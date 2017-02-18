//
// Created by Arseny Tolmachev on 2017/02/18.
//

#include "darts.h"
#include "darts_trie.h"
#include <algorithm>
#include <cstring>

namespace jumanpp {
namespace core {

using namespace jumanpp::core::impl;

void DoubleArrayBuilder::add(StringPiece key, int value) {
  immediate_.emplace_back(key, value);
}


DoubleArrayBuilder::DoubleArrayBuilder(): array_{std::make_unique<DoubleArray>()} {}

Status DoubleArrayBuilder::build() {
  std::sort(immediate_.begin(), immediate_.end(), [](const PieceWithValue& l, const PieceWithValue& r) {
    int cmp = std::strncmp(l.key.begin(), r.key.begin(), std::min(l.key.size(), r.key.size()));
    return cmp < 0;
  });

  std::vector<const char*> keys;
  std::vector<size_t> lengths;
  std::vector<i32> values;
  keys.reserve(immediate_.size());
  lengths.reserve(immediate_.size());
  values.reserve(immediate_.size());

  for (auto& o: immediate_) {
    keys.push_back(o.key.begin());
    lengths.push_back(o.key.size());
    values.push_back(o.value);
  }

  //immediate memory is no more needed, we can drop it
  immediate_.clear();
  immediate_.shrink_to_fit();

  auto retval = array_->build(keys.size(), keys.data(), lengths.data(), values.data());

  if (retval != 0) {
    return Status::InvalidState() << "double array build failed, code=" << retval;
  }

  return Status::Ok();
}

size_t DoubleArrayBuilder::underlyingByteSize() const {
  return array_->size();
}

const void *DoubleArrayBuilder::underlyingStorage() const {
  return array_->array();
}

//empty destructor is needed to hide darts interface completely
DoubleArrayBuilder::~DoubleArrayBuilder() {

}
}
}
