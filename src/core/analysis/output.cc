//
// Created by Arseny Tolmachev on 2017/03/01.
//

#include "output.h"
#include "core/analysis/extra_nodes.h"
#include "core/analysis/lattice_types.h"

namespace jumanpp {
namespace core {
namespace analysis {

Status OutputManager::stringField(StringPiece name, StringField *result) const {
  auto fld = holder_->fieldByName(name);
  if (fld == nullptr) {
    return JPPS_INVALID_PARAMETER << "dictionary field with name " << name
                                  << " was not found";
  }
  if (fld->columnType != spec::FieldType::String) {
    return JPPS_INVALID_PARAMETER << "field " << name
                                  << " was not string typed";
  }
  result->initialize(fld->idxInEntry, xtra_, fld->strings);
  return Status::Ok();
}

Status OutputManager::stringListField(StringPiece name,
                                      StringListField *result) const {
  auto fld = holder_->fieldByName(name);
  if (fld == nullptr) {
    return JPPS_INVALID_PARAMETER << "dictionary field with name " << name
                                  << " was not found";
  }
  if (fld->columnType != spec::FieldType::StringList) {
    return JPPS_INVALID_PARAMETER << "field " << name << " was not stringlist";
  }
  result->initialize(fld->idxInEntry, fld->postions, fld->strings);
  return Status::Ok();
}

Status OutputManager::kvListField(StringPiece name, KVListField *result) const {
  auto fld = holder_->fieldByName(name);
  if (fld == nullptr) {
    return JPPS_INVALID_PARAMETER << "dictionary field with name " << name
                                  << " was not found";
  }
  if (fld->columnType != spec::FieldType::StringKVList) {
    return JPPS_INVALID_PARAMETER << "field " << name << " was not kvlist";
  }
  result->initialize(fld->idxInEntry, fld->postions, fld->strings);
  return Status::Ok();
}

NodeWalker OutputManager::nodeWalker() const { return NodeWalker{}; }

bool OutputManager::locate(LatticeNodePtr ptr, NodeWalker *result) const {
  auto bnd = lattice_->boundary(ptr.boundary);
  auto eptr = bnd->entry(ptr.position);
  return locate(eptr, result);
}

bool OutputManager::locate(EntryPtr ptr, NodeWalker *result) const {
  result->mgr_ = this;
  result->current_ = EntryPtr::Invalid();

  result->buffer_.setCounts(static_cast<u32>(entries_.numFeatures()),
                            static_cast<u32>(entries_.numData()));

  result->current_ = ptr;
  if (ptr.isSpecial()) {
    auto node = xtra_->node(ptr);
    if (node == nullptr) {
      return false;
    }

    if (node->header.type == ExtraNodeType::Unknown) {
      auto actualPtr = node->header.unk.templatePtr;
      result->buffer_.fillFromStorage(actualPtr, this->entries_.entryData());
      auto data = xtra_->nodeContent(node);
      result->buffer_.overwriteFeaturesWith(data);
      return true;
    }

  } else {
    result->buffer_.fillFromStorage(result->current_,
                                    this->entries_.entryData());
    return true;
  }
  return false;
}

OutputManager::OutputManager(const ExtraNodesContext *xtra,
                             const dic::DictionaryHolder *holder,
                             const Lattice *lattice)
    : xtra_(xtra),
      holder_(holder),
      entries_{holder->entries()},
      lattice_(lattice) {}

i32 OutputManager::valueOfUnkPlaceholder(EntryPtr eptr,
                                         i32 placeholderIdx) const {
  JPP_DCHECK(eptr.isSpecial());
  return xtra_->placeholderData(eptr, placeholderIdx);
}

StringPiece StringField::operator[](const NodeWalker &node) const {
  i32 value = 0;
  if (node.valueOf(index_, &value)) {
    if (value < 0) {
      return xtra_->node(node.eptr())->header.unk.surface;
    }
    StringPiece result;
    if (reader_.value().readAt(value, &result)) {
      return result;
    }
    JPP_DCHECK_NOT("should fail in debug");
    return StringPiece{"----READ_ERROR!!!----"};
  }
  JPP_DCHECK_NOT("should fail in debug");
  return StringPiece{"-----STRING_FIELD_ERROR!!!----"};
}

void StringField::initialize(i32 index, const ExtraNodesContext *xtra,
                             dic::impl::StringStorageReader reader) {
  index_ = index;
  xtra_ = xtra;
  reader_.initialize(reader);
}

i32 StringField::pointer(const NodeWalker &node) const {
  i32 value = 0;
  node.valueOf(index_, &value);
  return value;
}

StringListIterator StringListField::operator[](const NodeWalker &node) const {
  i32 ptr = -1;
  auto status = node.valueOf(index_, &ptr);
  JPP_DCHECK(status);
  JPP_DCHECK_NE(ptr, -1);
  if (ptr == -1) {
    ptr = 0;
  }
  return StringListIterator{strings_.value(), ints_.value().listAt(ptr)};
}

void StringListField::initialize(
    i32 index, const dic::impl::IntStorageReader &ints,
    const dic::impl::StringStorageReader &strings) {
  this->index_ = index;
  this->ints_.initialize(ints);
  this->strings_.initialize(strings);
}

KVListIterator KVListField::operator[](const NodeWalker &node) const {
  i32 ptr = -1;
  auto status = node.valueOf(index_, &ptr);
  JPP_DCHECK(status);
  JPP_DCHECK_NE(ptr, -1);
  if (ptr == -1) {
    ptr = 0;
  }
  return KVListIterator{strings_.value(), ints_.value().kvListAt(ptr)};
}

void KVListField::initialize(i32 index, const dic::impl::IntStorageReader &ints,
                             const dic::impl::StringStorageReader &strings) {
  this->index_ = index;
  this->ints_.initialize(ints);
  this->strings_.initialize(strings);
}

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp