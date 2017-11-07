//
// Created by Arseny Tolmachev on 2017/03/01.
//

#include "dic_reader.h"
#include "core/analysis/extra_nodes.h"

namespace jumanpp {
namespace core {
namespace analysis {

util::MutableArraySlice<i32> DictNode::copyTo(ExtraNode *node) const {
  size_t sz = baseData_.size();
  util::MutableArraySlice<i32> slice{node->content, sz};
  std::copy(baseData_.begin(), baseData_.end(), slice.begin());
  return slice;
}

OwningDictNode DicReader::readEntry(EntryPtr ptr) const {
  i32 ipt = ptr.dicPtr();
  auto entrySize = (size_t)dic_.entries().numFeatures();
  OwningDictNode odn{entrySize};
  auto arraySlice = odn.data();
  JPP_DCHECK_EQ(arraySlice.size(), entrySize);
  dic_.entries().entryAtPtr(ipt).fill(arraySlice, entrySize);
  return odn;
}

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp
