//
// Created by Arseny Tolmachev on 2018/01/19.
//

#include "stream_reader.h"
#include <iostream>

namespace jumanpp {
namespace core {
namespace input {

Status PlainStreamReader::readExample(std::istream *stream) {
  comment_.clear();
  while (true) {
    input_.clear();
    std::getline(*stream, this->input_);
    if (input_.size() > 2 && input_[0] == '#' && input_[1] == ' ') {
      std::swap(comment_, input_);
    } else {
      break;
    }
  }

  if (comment_.size() > maxCommentLength_) {
    return Status::InvalidParameter()
           << "Comment size was: " << comment_.size()
           << " which is more than max: " << maxCommentLength_;
  }

  if (input_.size() > maxInputLength_) {
    return Status::InvalidParameter()
           << "Input size was: " << input_.size()
           << " which is more than max: " << maxInputLength_;
  }

  return Status::Ok();
}

}  // namespace input
}  // namespace core
}  // namespace jumanpp