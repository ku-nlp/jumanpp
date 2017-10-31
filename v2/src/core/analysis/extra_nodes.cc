//
// Created by Arseny Tolmachev on 2017/03/01.
//

#include "extra_nodes.h"
#include "core/analysis/dic_reader.h"
#include "util/stl_util.h"

namespace jumanpp {
namespace core {
namespace analysis {

ExtraNode *ExtraNodesContext::makeZeroedUnk() {
  auto node = allocateExtra();
  node->header.type = ExtraNodeType::Unknown;
  auto cont = nodeContent(node);
  util::fill(cont, 0);
  return node;
}

ExtraNode *ExtraNodesContext::makeUnk(const DictNode &pat) {
  auto node = allocateExtra();
  node->header.type = ExtraNodeType::Unknown;
  pat.copyTo(node);
  return node;
}

util::MutableArraySlice<i32> ExtraNodesContext::aliasBuffer(ExtraNode *node,
                                                            size_t numNodes) {
  auto buf = alloc_->allocateBuf<i32>(numNodes);
  node->header.alias.dictionaryNodes = buf;
  return buf;
}

ExtraNode *ExtraNodesContext::makeAlias() {
  auto node = allocateExtra();
  node->header.type = ExtraNodeType::Alias;
  return node;
}

ExtraNode *ExtraNodesContext::allocateExtra() {
  size_t memory =
      sizeof(ExtraNodeHeader) + sizeof(i32) * (numFields_ + numPlaceholders_);
  void *rawPtr = alloc_->allocate_memory(memory, alignof(ExtraNode));
  auto ptr = new (rawPtr) ExtraNode{};
  ptr->header.index = (i32)extraNodes_.size();
  //set placeholders to 0 (default value)
  std::fill(ptr->content + numFields_, ptr->content + numFields_ + numPlaceholders_, 0);
  extraNodes_.push_back(ptr);
  return ptr;
}

StringPiece ExtraNodesContext::unkString(i32 i) const {
  JPP_DCHECK_LT(i, 0);
  for (auto &node : extraNodes_) {  // linear search for the time being
    if (node->header.type == ExtraNodeType::Unknown) {
      if (node->header.unk.contentHash == i) {
        return node->header.unk.surface;
      }
    }
  }
  return StringPiece();
}

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp