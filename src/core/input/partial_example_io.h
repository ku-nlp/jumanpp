//
// Created by Arseny Tolmachev on 2018/01/19.
//

#ifndef JUMANPP_PARTIAL_EXAMPLE_IO_H
#define JUMANPP_PARTIAL_EXAMPLE_IO_H

#include "core/input/partial_example.h"
#include "util/csv_reader.h"

namespace jumanpp {
namespace core {
namespace input {

class PartialExampleReader {
  std::string filename_;
  TrainFieldsIndex* tio_;
  util::FlatMap<StringPiece, const TrainingExampleField*> fields_;
  util::FullyMappedFile file_;
  util::CsvReader csv_{'\t', '\0'};
  char32_t noBreakToken_ = U'&';
  std::vector<chars::InputCodepoint> codepts_;

 public:
  Status initialize(TrainFieldsIndex* tio, char32_t noBreakToken = U'&');
  Status readExample(PartialExample* result, bool* eof);
  Status openFile(StringPiece filename);
  Status setData(StringPiece data);
};

}  // namespace input
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_PARTIAL_EXAMPLE_IO_H
