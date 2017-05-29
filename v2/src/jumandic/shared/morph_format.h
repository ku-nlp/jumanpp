//
// Created by Arseny Tolmachev on 2017/05/24.
//

#ifndef JUMANPP_MORPH_FORMAT_H
#define JUMANPP_MORPH_FORMAT_H

#include "juman_format.h"

namespace jumanpp {
namespace jumandic {
namespace output {

class MorphFormat : public OutputFormat {
  JumandicFields fields_;
  util::io::Printer printer_;
  core::analysis::AnalysisResult analysisResult_;
  core::analysis::AnalysisPath top1_;
  std::array<i32, JumandicNumFields> fieldBuffer_;
  core::analysis::NodeWalker walker{&fieldBuffer_};
  bool fmrp_;

 public:
  MorphFormat(bool fmrp = false) : fmrp_{fmrp} {}
  Status initialize(const core::analysis::OutputManager& om);
  Status format(const core::analysis::Analyzer& analyzer, StringPiece comment);
  StringPiece result() const { return printer_.result(); }
};

}  // namespace output
}  // namespace jumandic
}  // namespace jumanpp

#endif  // JUMANPP_MORPH_FORMAT_H
