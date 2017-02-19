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

/**
 * This class allows to read csv files line by line.
 *
 * TODO: implement reading quoted and escaped data
 */
class CsvReader {
  MappedFile file_;
  MappedFileFragment fragment_;
  std::vector<StringPiece> fields_;
  std::vector<char> temp_;
  char separator_;
  char quote_;
  char escape_;
  const char* position_;
  const char* end_;
  i64 line_number_;

 public:
  CsvReader(char separator = ',', char quote = '"', char escape = '\\');
  Status open(const StringPiece& filename);
  Status initFromMemory(const StringPiece& data);
  bool nextLine();
  i32 numFields() const;
  StringPiece field(i32 idx) const;
  i64 lineNumber() const { return line_number_;  }
};
}
}

#endif  // JUMANPP_CSV_READER_H
