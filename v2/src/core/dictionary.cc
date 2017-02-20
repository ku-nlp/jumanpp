//
// Created by Arseny Tolmachev on 2017/02/19.
//

#include "dictionary.h"
#include "impl/dic_import.h"

#include <util/coded_io.h>
#include <util/csv_reader.h>
#include <util/flatmap.h>
#include <util/string_piece.h>
#include <memory>
#include <vector>

namespace jumanpp {
namespace core {
namespace dic {

Status EntryDescriptor::build(const DictionarySpec& spec) {
  spec_ = &spec;

  return Status::Ok();
}

Status DictionaryBuilder::importFromMemory(StringPiece data, StringPiece name) {
  util::CsvReader csv;

  i32 maxUsedCol = -1;
  StringPiece maxFieldName;
  std::vector<std::unique_ptr<impl::FieldImporter>> importers;
  for (const auto& column : spec_->columns) {
    auto colIdx = column.position - 1;

    auto tp = column.columnType;
    if (tp == ColumnType::String) {
      importers.emplace_back(new impl::StringFieldImporter{colIdx});
    } else {
      assert(false && "support other column types");
    }

    if (maxUsedCol < colIdx) {
      maxUsedCol = colIdx;
      maxFieldName = column.name;
    }
  }

  JPP_RETURN_IF_ERROR(csv.initFromMemory(data));

  while (csv.nextLine()) {
    auto ncols = csv.numFields();
    if (maxUsedCol >= ncols) {
      return Status::InvalidParameter()
             << "when processing file: " << name << " there were " << ncols
             << " columns, however field " << maxFieldName << " is defined as "
             << maxUsedCol << "th column";
    }

    for (auto& imp : importers) {
      imp->importFieldValue(csv);
    }
  }

  return Status::Ok();
}

}  // dic
}  // core
}  // jumanpp
