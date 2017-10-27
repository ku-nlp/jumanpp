//
// Created by Arseny Tolmachev on 2017/03/01.
//

#ifndef JUMANPP_DIC_READER_H
#define JUMANPP_DIC_READER_H

#include "core/dictionary.h"
#include "util/array_slice.h"
#include "util/memory.hpp"
#include "util/types.hpp"

namespace jumanpp {
namespace core {
namespace analysis {

struct ExtraNode;

class DictNode {
  util::ArraySlice<i32> baseData_;

 public:
  explicit DictNode(util::ArraySlice<i32> data) : baseData_{data} {}
  util::MutableArraySlice<i32> copyTo(ExtraNode* node) const;
  util::ArraySlice<i32> data() const { return baseData_; }
};

class OwningDictNode {
  std::vector<i32> baseData_;

 public:
  OwningDictNode(size_t size) : baseData_(size, 0) {}
  explicit OwningDictNode(util::ArraySlice<i32> data)
      : baseData_{data.begin(), data.end()} {}
  explicit OwningDictNode(const DictNode& node) : OwningDictNode(node.data()) {}
  operator DictNode() const {
    return DictNode{util::ArraySlice<i32>{baseData_}};
  }

  util::ArraySlice<i32> data() const { return baseData_; }
  util::MutableArraySlice<i32> data() { return &baseData_; }
  void resize(size_t size) { baseData_.resize(size); }
};

class DicReader {
  util::memory::PoolAlloc* alloc_;
  const dic::DictionaryHolder& dic_;

 public:
  DicReader(util::memory::PoolAlloc* alloc, const dic::DictionaryHolder& holder)
      : alloc_{alloc}, dic_{holder} {}
  OwningDictNode readEntry(EntryPtr ptr) const;
};

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_DIC_READER_H
