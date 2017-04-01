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
  util::ArraySlice<i32> baseData;

 public:
  DictNode(util::ArraySlice<i32> data) : baseData{data} {}
  util::MutableArraySlice<i32> copyTo(ExtraNode* node) const;
  util::ArraySlice<i32> data() const { return baseData; }
};

class OwningDictNode {
  std::vector<i32> baseData;

 public:
  OwningDictNode(util::ArraySlice<i32> data)
      : baseData{data.begin(), data.end()} {}
  OwningDictNode(const DictNode& node) : OwningDictNode(node.data()) {}
  operator DictNode() const {
    return DictNode{util::ArraySlice<i32>{baseData}};
  }
};

class DicReader {
  util::memory::ManagedAllocatorCore* alloc_;
  const dic::DictionaryHolder& dic_;

 public:
  DicReader(util::memory::ManagedAllocatorCore* alloc,
            const dic::DictionaryHolder& holder)
      : alloc_{alloc}, dic_{holder} {}
  DictNode readEntry(EntryPtr ptr) const;
};

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_DIC_READER_H
