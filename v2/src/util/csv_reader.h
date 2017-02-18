//
// Created by Arseny Tolmachev on 2017/02/18.
//

#ifndef JUMANPP_CSV_READER_H
#define JUMANPP_CSV_READER_H

#include <vector>
#include "mmap.h"
#include "string_piece.h"
#include "types.hpp"

namespace jumanpp {
namespace util {

class CsvReader {
  MappedFile file_;
  MappedFileFragment fragment_;
  std::vector<StringPiece> fields_;
  std::vector<char> temp_;
  char separator_;
  char quote_;
  char escape_;
  const char* position_;
  const char *end_;

 public:
  CsvReader(char separator = ',', char quote = '"', char escape = '\\');
  Status open(const StringPiece& filename);
  bool nextLine();
  i32 numFields() const;
  StringPiece field(i32 idx) const;


};
}
}

#endif  // JUMANPP_CSV_READER_H
