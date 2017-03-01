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
  DictNode(util::ArraySlice<i32> data): baseData{data} {}
  util::MutableArraySlice<i32> copyTo(ExtraNode* node) const;
};

class DicReader {
  util::memory::ManagedAllocatorCore* alloc_;
  dic::DictionaryHolder dic_;

public:
  DictNode readEntry(EntryPtr ptr) const;
};

}  // analysis
}  // core
}  // jumanpp

#endif  // JUMANPP_DIC_READER_H
