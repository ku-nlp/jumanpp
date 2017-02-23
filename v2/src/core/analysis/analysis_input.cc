///
// Created by Arseny Tolmachev on 2017/02/23.
//

#include "analysis_input.h"

namespace jumanpp {
namespace core {
namespace analysis {

Status AnalysisInput::reset(StringPiece data) {
  if (data.size() > max_size_) {
    return Status::InvalidParameter()
           << "byte size of input string (" << data.size()
           << ") is greater than maximum allowed (" << max_size_ << ")";
  }

  raw_input_.clear();
  codepoints_.clear();
  raw_input_.append(data.begin(), data.end());
  return chars::preprocessRawData(raw_input_, &codepoints_);
}

}  // analysis
}  // core
}  // jumanpp