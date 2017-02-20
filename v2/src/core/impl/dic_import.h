//
// Created by Arseny Tolmachev on 2017/02/20.
//

#ifndef JUMANPP_DIC_IMPORT_H
#define JUMANPP_DIC_IMPORT_H

#include <util/coded_io.h>
#include <util/csv_reader.h>
#include <util/flatmap.h>
#include <util/status.hpp>

namespace jumanpp {
namespace core {
namespace dic {
namespace impl {

class FieldImporter {
 public:
  virtual bool importFieldValue(const util::CsvReader& csv) = 0;
  virtual ~FieldImporter() {}
};

class StringFieldImporter : public FieldImporter {
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

  bool importFieldValue(const util::CsvReader& csv) override {
    auto sp = csv.field(field_);
    mapping_[sp] += 1;
    return true;
  }

  Status makeStorage(std::ostream& result);

  i32 pointer(const util::CsvReader& csv) {
    auto sp = csv.field(field_);
    return mapping_[sp];
  }
};

class StringDomainTraversal {
  util::CodedBufferParser parser_;
  i32 position_ = 0;

 public:
  StringDomainTraversal(const StringPiece& data) : parser_{data} {}

  bool hasNext() const noexcept { return parser_.remaining() != 0; }

  i32 position() const noexcept { return position_; }

  bool next(StringPiece* result) noexcept {
    position_ = parser_.position();
    bool ret = parser_.readStringPiece(result);
    return ret;
  }
};

}  // impl
}  // dic
}  // core
}  // jumanpp

#endif  // JUMANPP_DIC_IMPORT_H
