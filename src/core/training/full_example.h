//
// Created by Arseny Tolmachev on 2017/10/10.
//

#ifndef JUMANPP_FULL_EXAMPLE_H
#define JUMANPP_FULL_EXAMPLE_H

#include <string>
#include "core/analysis/extra_nodes.h"
#include "core/input/training_io.h"
#include "util/characters.h"
#include "util/csv_reader.h"
#include "util/sliceable_array.h"
#include "util/string_piece.h"

namespace jumanpp {
namespace core {
namespace training {

using core::input::ExampleInfo;
using core::input::TrainFieldsIndex;

struct ExampleNode {
  StringPiece surface;
  util::ArraySlice<i32> data;
  i32 position;
  i32 length;
};

class FullyAnnotatedExample {
  std::string surface_;
  std::vector<StringPiece> strings_;
  std::vector<i32> data_;
  std::vector<i32> lengths_;

  StringPiece filename_;
  std::string comment_;
  i64 line_;

  friend class FullExampleReader;

 public:
  StringPiece surface() const { return surface_; }

  util::Sliceable<i32> data() {
    return {&data_, data_.size() / lengths_.size(), lengths_.size()};
  }

  ExampleNode nodeAt(i32 idx) const {
    JPP_DCHECK_IN(idx, 0, lengths_.size());
    i32 codeptPos = 0;
    for (int i = 0; i < idx; ++i) {
      codeptPos += lengths_[i];
    }
    i32 negIdx = 0;
    auto exampleData = const_cast<FullyAnnotatedExample*>(this)->data();
    auto examRow = exampleData.row(idx);
    for (i32 exEntry : examRow) {
      if (exEntry < 0) {
        negIdx = exEntry;
      }
    }

    StringPiece sp{};
    if (negIdx < 0) {
      sp = strings_[~negIdx];
    }

    return ExampleNode{sp, examRow, codeptPos, lengths_[idx]};
  }

  i32 numNodes() const { return static_cast<i32>(lengths_.size()); }

  StringPiece comment() const { return comment_; }

  void reset() {
    comment_.clear();
    surface_.clear();
    strings_.clear();
    data_.clear();
    lengths_.clear();
  }

  void setInfo(StringPiece filename, i64 lineNo) {
    filename_ = filename;
    line_ = lineNo;
  }

  ExampleInfo exampleInfo() const {
    return ExampleInfo{filename_, comment_, line_};
  }
};

enum class DataReaderMode { SimpleCsv, DoubleCsv };

class FullExampleReader {
  const TrainFieldsIndex* tio_;
  DataReaderMode mode_;
  util::CsvReader csv_;
  util::CsvReader csv2_;
  bool finished_;
  std::vector<chars::InputCodepoint> codepts_;
  char doubleFldSep_;
  StringPiece filename_;
  util::CharBuffer<> charBuffer_;

  Status readSingleExampleFragment(const util::CsvReader& csv,
                                   FullyAnnotatedExample* result);

 public:
  void setTrainingIo(const TrainFieldsIndex* io) { tio_ = io; }

  Status initDoubleCsv(StringPiece data, char tokenSep = ' ',
                       char fieldSep = '_');

  Status initCsv(StringPiece data);
  bool finished() const { return finished_; }
  Status readFullExampleDblCsv(FullyAnnotatedExample* result);
  Status readFullExampleCsv(FullyAnnotatedExample* result);
  Status readFullExample(FullyAnnotatedExample* result);

  i64 lineNumber() const { return csv_.lineNumber(); }

  void resetInput(StringPiece data) {
    Status s = csv_.initFromMemory(data);
    JPP_DCHECK(s);
    charBuffer_.reset();
    finished_ = false;
  }

  void setFilename(StringPiece filename) { filename_ = filename; }

  StringPiece filename() const { return filename_; }
};

}  // namespace training
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_FULL_EXAMPLE_H
