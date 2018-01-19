//
// Created by Arseny Tolmachev on 2018/01/19.
//

#ifndef JUMANPP_STREAM_READER_H
#define JUMANPP_STREAM_READER_H

#include <iosfwd>
#include "core/analysis/analyzer.h"
#include "util/status.hpp"

namespace jumanpp {
namespace core {
namespace input {

class StreamReader {
 public:
  virtual Status readExample(std::istream* stream) = 0;
  virtual Status analyzeWith(analysis::Analyzer* an) = 0;
  virtual StringPiece comment() = 0;
  virtual ~StreamReader() = default;
};

class PlainStreamReader : public StreamReader {
  std::string input_;
  std::string comment_;
  u64 maxInputLength_ = 4096;
  u64 maxCommentLength_ = 4096;

 public:
  void setMaxSizes(u64 inputLength, u64 commentLength) {
    maxInputLength_ = inputLength;
    maxCommentLength_ = commentLength;
  }

  virtual Status readExample(std::istream* stream) override;
  virtual Status analyzeWith(analysis::Analyzer* an) override {
    JPP_RETURN_IF_ERROR(an->analyze(input_));
    return Status::Ok();
  }
  virtual StringPiece comment() override {
    if (comment_.size() < 2) {
      return EMPTY_SP;
    }
    return StringPiece{comment_}.from(2);
  }
};

}  // namespace input
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_STREAM_READER_H
