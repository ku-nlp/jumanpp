//
// Created by Arseny Tolmachev on 2017/03/01.
//

#include "dic_reader.h"
#include "core/analysis/extra_nodes.h"
#include "util/stl_util.h"

namespace jumanpp {
namespace core {
namespace analysis {

util::MutableArraySlice<i32> DictNode::copyTo(ExtraNode *node) const {
  size_t sz = this->entry_.numFeatures();
  util::MutableArraySlice<i32> slice{node->content, sz};
  util::copy_buffer(entry_.features(), slice);
  return slice;
}

DictNode DicReader::readEntry(EntryPtr ptr) const {
  DictNode node;
  auto dicEntries = dic_.entries();
  node.entry_.setCounts(static_cast<u32>(dicEntries.numFeatures()),
                        static_cast<u32>(dicEntries.numData()));
  JPP_DCHECK(node.entry_.fillFromStorage(ptr, dicEntries.entryData()));
  return node;
}

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp
