//
// Created by Arseny Tolmachev on 2017/02/26.
//

#ifndef JUMANPP_SOA_H
#define JUMANPP_SOA_H

#include "util/array_slice.h"
#include "util/memory.hpp"
#include "util/status.hpp"

namespace jumanpp {
namespace util {
namespace memory {

class ManagedObject {
 public:
  virtual ~ManagedObject() {}
};

class StructOfArraysBase;

class SOAField : public ManagedObject {
 protected:
  StructOfArraysBase* manager_;

 public:
  SOAField(StructOfArraysBase* manager) : manager_(manager) {}
  virtual size_t objSize() const = 0;
  virtual size_t requiredSize() const = 0;
  virtual size_t requiredAlign() const = 0;
  virtual void injectMemoty(void* memory) = 0;
};

struct SOAElementInfo {
  size_t objects;
  size_t objSize;
  size_t alignment;
  size_t offset;
  size_t byteSize;
};

class FieldUtil;

class StructOfArraysBase : public ManagedObject {
 protected:
  ManagedAllocatorCore* acore_;
  ManagedVector<SOAField*> fields_;
  ArraySlice<SOAElementInfo> dataInfo_;
  size_t totalSize_ = 0;
  size_t maxAlignment_ = 0;

  StructOfArraysBase(ManagedAllocatorCore* alloc)
      : acore_{alloc}, fields_{acore_} {
    fields_.reserve(10);
  }

  StructOfArraysBase(const StructOfArraysBase& o)
      : acore_{o.acore_},
        fields_{acore_},  // do NOT initialize fields
        dataInfo_{o.dataInfo_},
        totalSize_{o.totalSize_},
        maxAlignment_{o.maxAlignment_} {
    fields_.reserve(o.fields_.size());
  }

  Status initState() {
    if (fields_.size() == 0) {
      return Status::Ok();
    }

    auto infos = acore_->make<ManagedVector<SOAElementInfo>>(acore_);
    infos->resize(fields_.size());

    // compute basic info
    for (size_t i = 0; i < fields_.size(); ++i) {
      auto f = fields_[i];
      auto& nfo = infos->at(i);
      nfo.alignment = f->requiredAlign();
      nfo.objSize = f->objSize();
      nfo.objects = f->requiredSize() * this->arraySize();
      nfo.byteSize = nfo.objSize * nfo.objects;
    }

    // compute offsets
    size_t offset = 0;
    size_t maxAlignment = 0;
    for (size_t i = 0; i < fields_.size(); ++i) {
      auto& cur = infos->at(i);
      offset = Align(offset, cur.alignment);
      cur.offset = offset;
      offset += cur.byteSize;
      maxAlignment = std::max(maxAlignment, cur.alignment);
    }

    // initialize element info collection
    // infos vector is managed and its data is struct, so we do not care about
    // its deletion
    dataInfo_ = ArraySlice<SOAElementInfo>{infos->data(), infos->size()};

    totalSize_ = offset;
    maxAlignment_ = maxAlignment;

    return Status::Ok();
  }

  void registerField(SOAField* field) { fields_.push_back(field); }

 public:
  virtual size_t arraySize() const = 0;
  friend class FieldUtil;
};

class FieldUtil {
 public:
  static void regField(SOAField* fld, StructOfArraysBase* mgr) {
    mgr->registerField(fld);
  }
};

class StructOfArrays : public StructOfArraysBase {
  char* memory;
  size_t count_;

 public:
  StructOfArrays(ManagedAllocatorCore* alloc, size_t count)
      : StructOfArraysBase(alloc), count_{count} {}

  Status initialize() {
    JPP_RETURN_IF_ERROR(initState());

    // allocate memoty
    memory = reinterpret_cast<char*>(
        acore_->allocate_memory(totalSize_, maxAlignment_));

    for (int i = 0; i < fields_.size(); ++i) {
      auto f = fields_[i];
      auto& cur = dataInfo_[i];
      f->injectMemoty(memory + cur.offset);
    }

    return Status::Ok();
  }

  size_t arraySize() const override { return count_; }
};

template <typename Child>
class StructOfArraysFactory : public StructOfArraysBase {
  ManagedPtr<ManagedVector<Child>> children_;

 protected:
  size_t itemCount_ = 0;

  StructOfArraysFactory(ManagedAllocatorCore* alloc, size_t itemCount,
                        size_t appxChildren)
      : StructOfArraysBase(alloc), itemCount_(itemCount) {
    children_ = alloc->make_unique<ManagedVector<Child>>(alloc);
    children_->reserve(appxChildren);
  }

  StructOfArraysFactory(const StructOfArraysFactory<Child>& o)
      : StructOfArraysBase(o), itemCount_{o.itemCount_} {}

  Child& child(size_t idx) { return children_->at(idx); }

  const Child& child(size_t idx) const { return children_->at(idx); }

 private:
  void initChild(StructOfArraysFactory *child) {
    JPP_DCHECK_EQ(child->fields_.size(), fields_.size());
    auto mem = reinterpret_cast<char*>(
        acore_->allocate_memory(totalSize_, maxAlignment_));

    for (int i = 0; i < fields_.size(); ++i) {
      auto f = child->fields_[i];
      auto& nfo = dataInfo_[i];
      f->injectMemoty(mem + nfo.offset);
    }
  }

 public:
  Child& makeOne() {
    JPP_DCHECK(children_ && "can not make subobjects from subobjects");
    if (dataInfo_.size() != fields_.size()) {
      initState();
    }
    children_->emplace_back(static_cast<Child&>(*this));
    Child& c = children_->back();
    initChild(&c);
    return c;
  }

  Status initialize() {
    return initState();
  }

  size_t arraySize() const override { return itemCount_; }
};

template <typename T, size_t alignment = alignof(T)>
class SizedArrayField : public SOAField {
  MutableArraySlice<T> objects_;
  size_t rowCnt_;

 public:
  SizedArrayField(StructOfArraysBase* manager, size_t rows)
      : SOAField{manager}, rowCnt_{rows} {
    FieldUtil::regField(this, manager);
  }

  size_t objSize() const override { return sizeof(T); }

  size_t requiredSize() const override { return rowCnt_; }

  size_t requiredAlign() const override { return alignment; }

  void injectMemoty(void* memory) override {
    auto ptr = reinterpret_cast<T*>(memory);
    objects_ =
        util::MutableArraySlice<T>(ptr, requiredSize() * manager_->arraySize());
  }

  util::MutableArraySlice<T> row(size_t row) {
    JPP_DCHECK_NE(objects_.data(), nullptr);
    return util::MutableArraySlice<T>(objects_, rowCnt_ * row, rowCnt_);
  }

  util::ArraySlice<T> row(size_t row) const {
    JPP_DCHECK_NE(objects_.data(), nullptr);
    return util::ArraySlice<T>(objects_, rowCnt_ * row, rowCnt_);
  }

  util::MutableArraySlice<T> data() {
    JPP_DCHECK_NE(objects_.data(), nullptr);
    return objects_;
  }

  util::ArraySlice<T> data() const {
    JPP_DCHECK_NE(objects_.data(), nullptr);
    return objects_;
  }
};

}  // memory
}  // util
}  // jumanpp

#endif  // JUMANPP_SOA_H
