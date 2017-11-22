//
// Created by Arseny Tolmachev on 2017/02/20.
//

#ifndef JUMANPP_DIC_IMPORT_H
#define JUMANPP_DIC_IMPORT_H

#include <algorithm>
#include <regex>
#include "util/char_buffer.h"
#include "util/coded_io.h"
#include "util/csv_reader.h"
#include "util/flatmap.h"
#include "util/logging.hpp"
#include "util/lru_cache.h"
#include "util/status.hpp"

namespace jumanpp {
namespace core {
namespace dic {
namespace impl {

/**
 * Base interface for objects which import a single field from csv
 * into dictionary
 */
class FieldImporter {
 public:
  /**
   * Make sure that the string gets into the index and will be assigned a
   * pointer
   * @param sp
   * @return
   */
  virtual bool importString(StringPiece sp) = 0;

  /**
   * Import own field from csv row
   * @param csv
   * @return
   */
  virtual bool importFieldValue(const util::CsvReader& csv) = 0;

  /**
   * Create the storage for saving to disk
   * @param result
   * @return
   */
  virtual Status makeStorage(util::CodedBuffer* result) = 0;
  virtual void injectFieldBuffer(util::CodedBuffer* buffer){};
  virtual i32 fieldPointer(const util::CsvReader& csv) = 0;
  virtual i32 uniqueValues() const = 0;
  virtual ~FieldImporter() {}
};

class StringStorage {
  using Mapping = util::FlatMap<StringPiece, i32>;
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
  Mapping mapping_;
  util::CharBuffer<> contents_;
  i32 alignmentPower = 0;
  size_t alignment = 1;

 public:
  bool increaseFieldValueCount(StringPiece sp) {
    // the StringPiece could be transient if it was escaped
    // need to import it before doing anything, if there were none
    if (mapping_.count(sp) == 0) {
      JPP_RET_CHECK(contents_.import(&sp));
      mapping_[sp] = 1;
    } else {
      mapping_[sp] += 1;
    }
    return true;
  }

  Status makeStorage(util::CodedBuffer* result);

  i32 valueOf(StringPiece sp) const {
    auto it = mapping_.find(sp);
    if (it == mapping_.end()) {
      return -1;
    }
    return it->second;
  }

  void setAlignment(i32 power) {
    alignmentPower = power;
    alignment = static_cast<size_t>(1 << power);
  }

  size_t size() const { return mapping_.size(); }

  Mapping::const_iterator begin() const { return mapping_.begin(); }
  Mapping::const_iterator end() const { return mapping_.end(); }
};

class StringFieldImporter : public FieldImporter {
 protected:
  StringStorage* storage_;
  i32 field_;
  StringPiece ignore_;

 public:
  StringFieldImporter(StringStorage* storage, i32 field, StringPiece ignore)
      : storage_{storage}, field_{field}, ignore_{ignore} {}

  virtual bool importFieldValue(const util::CsvReader& csv) override {
    auto sp = csv.field(field_);
    if (ignore_ == sp || sp.size() == 0) {
      return true;
    }
    return storage_->increaseFieldValueCount(sp);
  }

  // This is used only in tests,
  // Probably should remove in future
  Status makeStorage(util::CodedBuffer* result) final;

  virtual i32 fieldPointer(const util::CsvReader& csv) override {
    auto sp = csv.field(field_);
    if (sp.size() == 0 || ignore_ == sp) {
      return 0;
    }
    JPP_CAPTURE(sp);
    JPP_CAPTURE(field_);
    auto val = storage_->valueOf(sp);
    JPP_DCHECK_NE(val, -1);
    return val;
  }

  virtual i32 uniqueValues() const override { return (i32)storage_->size(); }

  bool importString(StringPiece sp) override {
    return storage_->increaseFieldValueCount(sp);
  }
};

class StringListFieldImporter : public StringFieldImporter {
  /**
   * This will hold individual list data
   */
  util::CodedBuffer* buffer_;
  std::vector<i32> values_;

 public:
  StringListFieldImporter(StringStorage* storage, i32 field, StringPiece ignore)
      : StringFieldImporter::StringFieldImporter(storage, field, ignore) {}

  bool importFieldValue(const util::CsvReader& csv) override;

  void injectFieldBuffer(util::CodedBuffer* buffer) override;

  i32 fieldPointer(const util::CsvReader& csv) override;
};

class StringKeyValueListFieldImporter : public StringFieldImporter {
  util::CodedBuffer* buffer_ = nullptr;
  util::CodedBuffer local_;
  std::vector<std::pair<i32, i32>> values_{};
  util::CharBuffer<> charBuf_;
  util::LruCache<StringPiece, i32> positionCache_;
  std::string entrySeparator_;
  std::string kvSeparator_;

 public:
  StringKeyValueListFieldImporter(StringStorage* storage, i32 field,
                                  StringPiece ignore,
                                  StringPiece entrySeparator = StringPiece{" "},
                                  StringPiece kvSeparator = StringPiece{":"})
      : StringFieldImporter::StringFieldImporter(storage, field, ignore),
        positionCache_{50000},
        entrySeparator_{entrySeparator.str()},
        kvSeparator_{kvSeparator.str()} {}

  void injectFieldBuffer(util::CodedBuffer* buffer) override;

  bool importFieldValue(const util::CsvReader& csv) override;

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

class IntFieldImporter : public FieldImporter {
  i32 fld_;
  std::regex re_;
  std::match_results<const char*> mr_;

 public:
  IntFieldImporter(i32 field);

  bool importString(StringPiece sp) override;

  bool importFieldValue(const util::CsvReader& csv) override {
    StringPiece sp = csv.field(fld_);
    return importString(sp);
  }

  Status makeStorage(util::CodedBuffer* result) override {
    return Status::Ok();
  }

  i32 fieldPointer(const util::CsvReader& csv) override;

  i32 uniqueValues() const override { return 0; }
};

}  // namespace impl
}  // namespace dic
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_DIC_IMPORT_H
