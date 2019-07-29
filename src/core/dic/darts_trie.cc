//
// Created by Arseny Tolmachev on 2017/02/18.
//

#include "darts_trie.h"
#include <util/mmap.h>
#include <algorithm>
#include <cstring>
#include "darts.h"

namespace jumanpp {
namespace core {
namespace dic {

void DoubleArrayBuilder::add(StringPiece key, int value) {
  immediate_.emplace_back(key, value);
}

DoubleArrayBuilder::DoubleArrayBuilder()
    : array_{std::make_unique<impl::DoubleArrayCore>()} {}

Status DoubleArrayBuilder::build(ProgressCallback *progress) {
  std::sort(immediate_.begin(), immediate_.end(),
            [](const impl::PieceWithValue &l, const impl::PieceWithValue &r) {
              int cmp = std::strncmp(l.key.char_begin(), r.key.char_begin(),
                                     std::min(l.key.size(), r.key.size()));
              if (cmp == 0) return l.key.size() < r.key.size();
              return cmp < 0;
            });

  std::vector<StringPiece::pointer_t> keys;
  std::vector<size_t> lengths;
  std::vector<i32> values;
  keys.reserve(immediate_.size());
  lengths.reserve(immediate_.size());
  values.reserve(immediate_.size());

  for (auto &o : immediate_) {
    keys.push_back(o.key.begin());
    lengths.push_back(o.key.size());
    values.push_back(o.value);
  }

  // immediate memory is no more needed, we can drop it
  immediate_.clear();
  immediate_.shrink_to_fit();

  try {
    auto retval = array_->build(keys.size(), keys.data(), lengths.data(),
                                values.data(), progress);

    if (retval != 0) {
      return JPPS_INVALID_STATE << "double array build failed, code=" << retval;
    }
  } catch (JppDarts::Details::Exception& e) {
    return JPPS_INVALID_STATE << "failed to build darts trie, " << e.what();
  }

  return Status::Ok();
}

size_t DoubleArrayBuilder::underlyingByteSize() const {
  return array_->total_size();
}

const void *DoubleArrayBuilder::underlyingStorage() const {
  return array_->array();
}

// empty destructor is needed to hide darts interface completely
DoubleArrayBuilder::~DoubleArrayBuilder() {}

Status DoubleArray::loadFromMemory(StringPiece memory) {
  underlying_.reset(new impl::DoubleArrayCore{});
  auto &arr = *underlying_;

  // boo, this casts away const
  void *ptr = (void *)memory.begin();
  arr.set_array(ptr, memory.size() / arr.unit_size());

  return Status::Ok();
}

DoubleArray::~DoubleArray() {}

DoubleArray::DoubleArray() {}

StringPiece DoubleArray::contents() const {
  auto storage = reinterpret_cast<StringPiece::pointer_t>(underlying_->array());
  return StringPiece{storage, underlying_->total_size()};
}

void DoubleArray::plunder(DoubleArrayBuilder *bldr) {
  underlying_ = std::move(bldr->array_);
}

std::string DoubleArray::describe() const {
  std::string description;
  description += "size=";
  description += std::to_string(underlying_->size());
  return description;
}

TraverseStatus DoubleArrayTraversal::step(StringPiece data) {
  key_pos_ = 0;
  auto status = base_->traverse(data.begin(), node_pos_, key_pos_, data.size());

  switch (status) {
    case -1:
      return TraverseStatus::NoLeaf;
    case -2:
      return TraverseStatus::NoNode;
    default:
      value_ = status;
      return TraverseStatus::Ok;
  }
}

}  // namespace dic
}  // namespace core
}  // namespace jumanpp
