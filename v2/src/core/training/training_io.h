//
// Created by Arseny Tolmachev on 2017/03/22.
//

#ifndef JUMANPP_TRAINING_IO_H
#define JUMANPP_TRAINING_IO_H

#include <vector>
#include "core/core.h"
#include "core/spec/spec_types.h"
#include "util/flatmap.h"
#include "util/string_piece.h"

namespace jumanpp {
namespace core {
namespace training {

struct ExampleInfo {
  StringPiece file;
  StringPiece comment;
  i64 line;
};

struct TrainingExampleField {
  StringPiece name;
  util::FlatMap<StringPiece, i32>* str2int;
  i32 dicFieldIdx;
  i32 exampleFieldIdx;
};

class TrainingIo {
  std::vector<util::FlatMap<StringPiece, i32>> storages_;
  std::vector<TrainingExampleField> fields_;
  i32 surfaceFieldIdx_;

 public:
  const util::FlatMap<StringPiece, i32>& strings(int idx) const {
    return storages_[idx];
  }

  util::ArraySlice<TrainingExampleField> fields() const { return fields_; }

  i32 surfaceFieldIdx() const { return surfaceFieldIdx_; }

  Status initialize(const spec::TrainingSpec& spec, const CoreHolder& core);
};

}  // namespace training
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_TRAINING_IO_H
