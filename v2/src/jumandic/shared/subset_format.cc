//
// Created by Arseny Tolmachev on 2017/07/20.
//

#include "subset_format.h"

namespace jumanpp {
namespace jumandic {
namespace output {

Status SubsetFormat::format(const core::analysis::Analyzer &analyzer,
                            StringPiece comment) {
  JPP_RETURN_IF_ERROR(morph_.format(analyzer, comment));
  JPP_RETURN_IF_ERROR(mdic_.format(analyzer, comment));
  auto morphRes = morph_.result();
  buffer_.clear();
  buffer_.append("#### MRPH output ####\n");
  buffer_.append(morphRes.begin(), morphRes.end());
  buffer_.append("\n");
  buffer_.append("\n");

  auto mdicRes = mdic_.result();
  buffer_.append("### SUBSET OF DICTIONARY\n");
  buffer_.append(mdicRes.begin(), mdicRes.end());
  return Status::Ok();
}

}  // namespace output
}  // namespace jumandic
}  // namespace jumanpp