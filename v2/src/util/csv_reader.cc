//
// Created by Arseny Tolmachev on 2017/02/18.
//

#include "csv_reader.h"

namespace jumanpp {
namespace util {

CsvReader::CsvReader(char separator_, char quote_, char escape_)
    : separator_(separator_), quote_(quote_), escape_(escape_) {}

Status CsvReader::open(const StringPiece &filename) {
  JPP_RETURN_IF_ERROR(file_.open(filename, MMapType::ReadOnly));
  JPP_RETURN_IF_ERROR(file_.map(&fragment_, 0, file_.size()));
  position_ = reinterpret_cast<const char*>(fragment_.address());
  end_ = position_ + file_.size();
  return Status::Ok();
}

bool CsvReader::nextLine() {
  auto end = end_;
  auto position = position_;
  if (position == end) return false;
  fields_.clear();

  auto field_start = position;

  for (; ; ++position) {
    auto ch = *position;
    if (ch == '\n') {
      fields_.emplace_back(field_start, position);
      //next call starts from the next string, skipping EOL
      position_ = position + 1;
      break;
    }

    if (ch == separator_) {
      fields_.emplace_back(field_start, position);
      field_start = position + 1;
      continue;
    }

    //This block executes when end of file is reached
    if (position == end) {
      if (field_start != position) {
        fields_.emplace_back(field_start, position);
        position_ = position;
      }
      break;
    }
  }

  return !fields_.empty();
}

i32 CsvReader::numFields() const { return static_cast<i32>(fields_.size()); }

StringPiece CsvReader::field(i32 idx) const { return fields_[idx]; }

}
}
