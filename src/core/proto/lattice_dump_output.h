//
// Created by Arseny Tolmachev on 2017/12/08.
//

#ifndef JUMANPP_LATTICE_DUMP_OUTPUT_H
#define JUMANPP_LATTICE_DUMP_OUTPUT_H

#include "core/env.h"

namespace jumanpp {
namespace core {
namespace output {

struct LatticeDumpOutputImpl;

class LatticeDumpOutput : public OutputFormat {
  std::unique_ptr<LatticeDumpOutputImpl> impl_;

 public:
  LatticeDumpOutput();
  ~LatticeDumpOutput() override;
  Status initialize(const analysis::AnalyzerImpl* impl,
                    const analysis::WeightBuffer* weights);
  Status format(const core::analysis::Analyzer& analyzer,
                StringPiece comment) override;
  StringPiece result() const override;
};

}  // namespace output
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_LATTICE_DUMP_OUTPUT_H
