///
// Created by Arseny Tolmachev on 2017/02/23.
//

#include "analysis_input.h"
#include "lattice_builder.h"

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
  JPP_RETURN_IF_ERROR(chars::preprocessRawData(raw_input_, &codepoints_));

  constexpr auto max_codepoints = std::numeric_limits<LatticePosition>::max();
  if (codepoints().size() > max_codepoints) {
    return Status::InvalidState()
           << "jpp_core at current settings cannot process more than "
           << max_codepoints << "characters at once";
  }

  return Status::Ok();
}

LatticePosition AnalysisInput::numCodepoints() {
  return static_cast<LatticePosition>(codepoints_.size());
}

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp