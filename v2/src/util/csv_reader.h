//
// Created by Arseny Tolmachev on 2017/02/18.
//

#ifndef JUMANPP_CSV_READER_H
#define JUMANPP_CSV_READER_H

#include <vector>
#include "char_buffer.h"
#include "mmap.h"
#include "string_piece.h"
#include "types.hpp"

namespace jumanpp {
namespace util {

/**
 * This class allows to read Comma Separated Values (CSV) files line by line.
 *
 * We use https://tools.ietf.org/html/rfc4180 to define CSV format.
 *
 * File consits of fields, and separators.
 *
 * Lines of file are separated either by "\n" or by "\r\n".
 *
 * Default separator is comma (,).
 *
 * If the field contains comma it must be enclosed with quote character (default
 * = ").
 *
 * If the field contains quote character it must be enclosed with quote
 * character and all
 * quote characters must be escaped.
 *
 * Test csv:
 *
 * a,b,c
 * a,"b,c",c
 * a,"b""c",c
 * """",a,b
 * ,,c
 * ,"",a
 *
 * First line is normal.
 * Second line contains quoted string.
 * Third line contains quoted string with escaped quote => actual value of the
 * second field is {b"c}.
 * Fourth line contains a single quote charater in the first field.
 * Fields can also be empty like in fifth line.
 * The sixth line contains empty quoted string, it is also allowed.
 *
 * StringPieces that are returned contain already unescaped strings.
 * They are valid only until next nextLine() call.
 * You need to copy StringPiece somewhere if you want to store its contents.
 *
 * CharBuffer class is created for that objective.
 *
 */
class CsvReader {
  MappedFile file_;
  MappedFileFragment fragment_;
  std::vector<StringPiece> fields_;
  CharBuffer<16 * 1024> temp_;
  char separator_;
  char quote_;
  const char* position_;
  const char* end_;
  i64 line_number_;

  /**
   * @param position
   * @param result
   * @return true if quoted string had contained escaped quote characters
   */
  bool handleQuote(const char* position, StringPiece* result);
  bool unescapeString(StringPiece sp, StringPiece* result);

 public:
  CsvReader(char separator = ',', char quote = '"');
  Status open(const StringPiece& filename);
  Status initFromMemory(const StringPiece& data);
  bool nextLine();
  i32 numFields() const;
  StringPiece field(i32 idx) const;
  i64 lineNumber() const { return line_number_; }
};
}
}

#endif  // JUMANPP_CSV_READER_H
