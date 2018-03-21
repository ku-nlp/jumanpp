//
// Created by Arseny Tolmachev on 2017/12/08.
//

#ifndef JUMANPP_LATTICE_DUMP_OUTPUT_H
#define JUMANPP_LATTICE_DUMP_OUTPUT_H

#include "core/env.h"

namespace jumanpp {
class LatticeDump;
namespace core {
namespace output {

struct LatticeDumpOutputImpl;

class LatticeDumpOutput : public OutputFormat {
  bool fillFeatures_;
  bool fillBuffer_;
  std::unique_ptr<LatticeDumpOutputImpl> impl_;

 public:
  LatticeDumpOutput(bool fillFeatures, bool fillBuffer);
  ~LatticeDumpOutput() override;
  Status initialize(const analysis::AnalyzerImpl* impl,
                    const analysis::WeightBuffer* weights);
  Status format(const core::analysis::Analyzer& analyzer,
                StringPiece comment) override;
  StringPiece result() const override;
  const LatticeDump* objectPtr() const;

  bool wasInitialized() const { return static_cast<bool>(impl_); }
};

}  // namespace output
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_LATTICE_DUMP_OUTPUT_H
