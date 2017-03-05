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

using namespace jumanpp::core::impl;

void DoubleArrayBuilder::add(StringPiece key, int value) {
  immediate_.emplace_back(key, value);
}

DoubleArrayBuilder::DoubleArrayBuilder()
    : array_{std::make_unique<DoubleArrayCore>()} {}

Status DoubleArrayBuilder::build() {
  std::sort(immediate_.begin(), immediate_.end(),
            [](const PieceWithValue& l, const PieceWithValue& r) {
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

  for (auto& o : immediate_) {
    keys.push_back(o.key.begin());
    lengths.push_back(o.key.size());
    values.push_back(o.value);
  }

  // immediate memory is no more needed, we can drop it
  immediate_.clear();
  immediate_.shrink_to_fit();

  auto retval =
      array_->build(keys.size(), keys.data(), lengths.data(), values.data());

  if (retval != 0) {
    return Status::InvalidState() << "double array build failed, code="
                                  << retval;
  }

  return Status::Ok();
}

size_t DoubleArrayBuilder::underlyingByteSize() const { return array_->total_size(); }

const void* DoubleArrayBuilder::underlyingStorage() const {
  return array_->array();
}

// empty destructor is needed to hide darts interface completely
DoubleArrayBuilder::~DoubleArrayBuilder() {}

namespace impl {
struct DoubleArrayBackingFile {
  jumanpp::util::MappedFile file;
  jumanpp::util::MappedFileFragment fragment;
};
}

Status DoubleArray::loadFromMemory(StringPiece memory) {
  underlying_.reset(new DoubleArrayCore());
  auto& arr = *underlying_;

  // boo, this casts away const
  void* ptr = (void*)memory.begin();
  arr.set_array(ptr, memory.size());

  return Status::Ok();
}

Status DoubleArray::loadFromFile(StringPiece filename, size_t offset,
                                 size_t length) {
  file_.reset(new DoubleArrayBackingFile());
  auto& file = file_->file;
  auto& fragment = file_->fragment;

  JPP_RETURN_IF_ERROR(file.open(filename, jumanpp::util::MMapType::ReadOnly));
  if (length == InvalidSize) {
    length = file_->file.size() - offset;
  }
  JPP_RETURN_IF_ERROR(file.map(&fragment, offset, length));
  return loadFromMemory(fragment.asStringPiece());
}

DoubleArray::~DoubleArray() {}
DoubleArray::DoubleArray() {}

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

}  // namespace core
}  // namespace jumanpp
