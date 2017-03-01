//
// Created by Arseny Tolmachev on 2017/03/01.
//

#include "dic_reader.h"
#include "core/analysis/extra_nodes.h"

namespace jumanpp {
namespace core {
namespace analysis {

util::MutableArraySlice<i32> DictNode::copyTo(ExtraNode *node) const {
  size_t sz = baseData.size();
  util::MutableArraySlice<i32> slice{node->content, sz};
  std::copy(baseData.begin(), baseData.end(), slice.begin());
  return slice;
}

DictNode DicReader::readEntry(EntryPtr ptr) const {
  JPP_DCHECK_NOT(isSpecial(ptr));
  i32 ipt = idxFromEntryPtr(ptr);
  auto entrySize = (size_t) dic_.entries().entrySize();
  auto memory = alloc_->allocateArray<i32>(entrySize);
  util::MutableArraySlice<i32> slice {memory, entrySize};
  dic_.entries().entryAtPtr(ipt).fill(slice, entrySize);
  return DictNode {slice};
}

} // analysis
} // core
} // jumanpp


