//
// Created by Arseny Tolmachev on 2017/02/23.
//

#ifndef JUMANPP_ANALYSIS_INPUT_H
#define JUMANPP_ANALYSIS_INPUT_H

#include <string>
#include "util/array_slice.h"
#include "util/characters.h"
#include "util/status.hpp"
#include "util/types.hpp"

namespace jumanpp {
namespace core {
namespace analysis {

class AnalysisInput {
  size_t max_size_;
  std::string raw_input_;
  using CodepointStorage = std::vector<jumanpp::chars::InputCodepoint>;
  CodepointStorage codepoints_;

 public:
  AnalysisInput(size_t maxSize = 8 * 1024) : max_size_{maxSize} {
    raw_input_.reserve(maxSize);
  }

  Status reset(StringPiece data);

  const CodepointStorage &codepoints() const { return codepoints_; }

  u16 numCodepoints();

  StringPiece surface(i32 start, i32 end) const {
    if (start == end) {
      return EMPTY_SP;
    }
    JPP_DCHECK_LT(start, end);
    auto &startc = codepoints_[start];
    auto &endc = codepoints_[end - 1];
    auto s = startc.bytes.begin();
    auto e = endc.bytes.end();
    return StringPiece{s, e};
  }
};

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_ANALYSIS_INPUT_H
