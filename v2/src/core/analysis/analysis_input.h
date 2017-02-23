//
// Created by Arseny Tolmachev on 2017/02/23.
//

#ifndef JUMANPP_ANALYSIS_INPUT_H
#define JUMANPP_ANALYSIS_INPUT_H

#include <string>
#include "util/array_slice.h"
#include "util/characters.hpp"
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

  const CodepointStorage& codepoints() const { return codepoints_; }
};

}  // analysis
}  // core
}  // jumanpp

#endif  // JUMANPP_ANALYSIS_INPUT_H
