//
// Created by Arseny Tolmachev on 2017/02/20.
//

#ifndef JUMANPP_DIC_IMPORT_H
#define JUMANPP_DIC_IMPORT_H

#include <util/coded_io.h>
#include <util/csv_reader.h>
#include <util/flatmap.h>
#include <algorithm>
#include <array>
#include <util/status.hpp>

namespace jumanpp {
namespace core {
namespace dic {
namespace impl {

class FieldImporter {
 public:
  virtual bool importFieldValue(const util::CsvReader& csv) = 0;
  virtual Status makeStorage(util::CodedBuffer* result) = 0;
  virtual bool requiresFieldBuffer() const { return false; };
  virtual void injectFieldBuffer(util::CodedBuffer* buffer){};
  virtual i32 fieldPointer(const util::CsvReader& csv) = 0;
  virtual i32 uniqueValues() const = 0;
  virtual ~FieldImporter() {}
};

template <size_t PageSize = (1 << 20)>  // 1M by default
class CharBuffer {
  static constexpr ptrdiff_t page_size = PageSize;
  using page = std::array<char, page_size>;
  std::vector<page*> storage_;

  page* current_ = nullptr;
  ptrdiff_t position_ = page_size;

  bool ensure_size(size_t sz) {
    if (sz > page_size) {
      return false;
    }
    ptrdiff_t remaining = page_size - position_;
    if (remaining < sz) {
      auto ptr = new page;
      current_ = ptr;
      position_ = 0;
      storage_.push_back(ptr);
    }

    return true;
  }

 public:
  ~CharBuffer() {
    for (auto p : storage_) {
      delete p;
    }
  }

  bool import(StringPiece* sp) {
    auto psize = sp->size();
    JPP_RET_CHECK(ensure_size(psize));
    auto begin = current_->data() + position_;
    std::copy(sp->begin(), sp->end(), begin);
    *sp = StringPiece{begin, begin + psize};
    position_ += psize;
    return true;
  }
};

class StringFieldImporter : public FieldImporter {
 protected:
  /**
   * This hashmap has two usages in dictionary import.
   *
   * Initially it stores counts of each token of a field.
   *
   * However after the initial pass over the dictionary is over
   * and field storage is constructed, the map values are replaced by
   * positions of entries in the field storage.
   *
   */
  jumanpp::util::FlatMap<StringPiece, i32> mapping_;
  CharBuffer<> contents_;
  i32 field_;

  bool increaseFieldValueCount(StringPiece sp) {
    if (mapping_.count(sp) == 0) {
      JPP_RET_CHECK(contents_.import(&sp));
      mapping_[sp] = 1;
    } else {
      mapping_[sp] += 1;
    }
    return true;
  }

 public:
  StringFieldImporter(i32 field) : field_{field} {}

  virtual bool importFieldValue(const util::CsvReader& csv) override {
    auto sp = csv.field(field_);
    return increaseFieldValueCount(sp);
  }

  Status makeStorage(util::CodedBuffer* result) override;

  virtual i32 fieldPointer(const util::CsvReader& csv) override {
    auto sp = csv.field(field_);
    return mapping_[sp];
  }

  virtual i32 uniqueValues() const override { return (i32)mapping_.size(); }
};

class StringListFieldImporter : public StringFieldImporter {
  util::CodedBuffer* buffer_;
  std::vector<i32> values_;

 public:
  StringListFieldImporter(i32 field)
      : StringFieldImporter::StringFieldImporter(field) {}

  bool importFieldValue(const util::CsvReader& csv) override;

  virtual bool requiresFieldBuffer() const override { return true; };

  virtual void injectFieldBuffer(util::CodedBuffer* buffer) override {
    buffer_ = buffer;
  };

  i32 fieldPointer(const util::CsvReader& csv) override;
};

template <typename C>
void writePtrsAsDeltas(C& values, util::CodedBuffer& buffer) {
  // compute delta representation for field values
  std::sort(std::begin(values), std::end(values));
  for (i32 i = static_cast<i32>(values.size()) - 1; i > 0; --i) {
    auto right = values[i];
    auto left = values[i - 1];
    values[i] = right - left;
  }

  buffer.writeVarint(values.size());
  for (auto v : values) {
    JPP_DCHECK_GE(v, 0);  // should not have negative values here
    buffer.writeVarint(static_cast<u64>(v));
  }
}

}  // impl
}  // dic
}  // core
}  // jumanpp

#endif  // JUMANPP_DIC_IMPORT_H
