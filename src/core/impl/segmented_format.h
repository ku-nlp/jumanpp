//
// Created by Arseny Tolmachev on 2017/12/07.
//

#ifndef JUMANPP_SURFACE_ONLY_FORMAT_H
#define JUMANPP_SURFACE_ONLY_FORMAT_H

#include "core/analysis/analysis_result.h"
#include "core/analysis/output.h"
#include "core/env.h"
#include "util/fast_printer.h"

namespace jumanpp {
namespace core {
namespace output {

class SegmentedFormat : public OutputFormat {
  analysis::StringField surface_;
  const analysis::OutputManager* mgr_;
  std::string separator_;
  util::io::FastPrinter printer_;
  core::analysis::AnalysisPath top1_;

 public:
  Status initialize(const analysis::OutputManager& mgr,
                    const core::CoreHolder& core, StringPiece separator = " ");
  Status format(const core::analysis::Analyzer& analyzer,
                StringPiece comment) override;
  StringPiece result() const override;
};

}  // namespace output
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_SURFACE_ONLY_FORMAT_H
