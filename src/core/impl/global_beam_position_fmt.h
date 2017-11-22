//
// Created by Arseny Tolmachev on 2017/10/25.
//

#ifndef JUMANPP_GLOBAL_BEAM_POSITION_FMT_H
#define JUMANPP_GLOBAL_BEAM_POSITION_FMT_H

#include "core/analysis/output.h"
#include "core/env.h"
#include "util/printer.h"

namespace jumanpp {
namespace core {
namespace output {

class GlobalBeamPositionFormat : public OutputFormat {
  util::io::Printer printer_;
  analysis::StringField surface_;
  i32 maxElems_;

 public:
  explicit GlobalBeamPositionFormat(i32 maxElems) : maxElems_{maxElems} {}

  Status initialize(const core::analysis::Analyzer &analyzer);

  Status format(const core::analysis::Analyzer &analyzer,
                StringPiece comment) override;

  StringPiece result() const override { return printer_.result(); }
};

}  // namespace output
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_GLOBAL_BEAM_POSITION_FMT_H
