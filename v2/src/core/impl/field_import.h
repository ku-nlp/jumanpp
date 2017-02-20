//
// Created by Arseny Tolmachev on 2017/02/20.
//

#ifndef JUMANPP_DIC_IMPORT_H
#define JUMANPP_DIC_IMPORT_H

#include <util/coded_io.h>
#include <util/csv_reader.h>
#include <util/flatmap.h>
#include <algorithm>
#include <util/status.hpp>

namespace jumanpp {
namespace core {
namespace dic {
namespace impl {

class FieldImporter {
 public:
  virtual bool importFieldValue(const util::CsvReader& csv) = 0;
  virtual Status makeStorage(std::ostream& result) = 0;
  virtual i32 fieldPointer(const util::CsvReader& csv) = 0;
  virtual ~FieldImporter() {}
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
  i32 field_;

 public:
  StringFieldImporter(i32 field) : field_{field} {}

  virtual bool importFieldValue(const util::CsvReader& csv) override {
    auto sp = csv.field(field_);
    mapping_[sp] += 1;
    return true;
  }

  Status makeStorage(std::ostream& result) override;

  virtual i32 fieldPointer(const util::CsvReader& csv) override {
    auto sp = csv.field(field_);
    return mapping_[sp];
  }
};

class StringListFieldImporter : public StringFieldImporter {
  std::ostream& result_;
  util::CodedBuffer buffer_;
  std::vector<i32> values_;
  i32 position_ = 0;

 public:
  StringListFieldImporter(i32 field, std::ostream& result)
      : StringFieldImporter::StringFieldImporter(field), result_{result} {}

  bool importFieldValue(const util::CsvReader& csv) override {
    auto sp = csv.field(field_);
    while (sp.size() > 0) {
      auto space = std::find(sp.begin(), sp.end(), ' ');
      auto key = StringPiece{sp.begin(), space};
      mapping_[key] += 1;
      if (space == sp.end()) {
        return true;
      }
      sp = StringPiece{space + 1, sp.end()};
    }

    return true;
  }

  i32 fieldPointer(const util::CsvReader& csv) override {
    auto sp = csv.field(field_);

    values_.clear();

    while (sp.size() > 0) {
      auto space = std::find(sp.begin(), sp.end(), ' ');
      auto key = StringPiece{sp.begin(), space};
      values_.push_back(mapping_[key]);
      if (space == sp.end()) {
        break;
      }
      sp = StringPiece{space + 1, sp.end()};
    }

    // compute delta representation for field values
    std::sort(values_.begin(), values_.end());
    for (i32 i = static_cast<i32>(values_.size()) - 1; i > 0; --i) {
      auto right = values_[i];
      auto left = values_[i - 1];
      values_[i] = right - left;
    }

    buffer_.reset();
    buffer_.writeVarint(values_.size());
    for (auto v : values_) {
      JPP_DCHECK_GE(v, 0);  // should not have negative values here
      buffer_.writeVarint(static_cast<u64>(v));
    }

    auto data = buffer_.contents();
    result_ << data;

    i32 result = position_;
    position_ += data.size();

    return result;
  }
};

}  // impl
}  // dic
}  // core
}  // jumanpp

#endif  // JUMANPP_DIC_IMPORT_H
