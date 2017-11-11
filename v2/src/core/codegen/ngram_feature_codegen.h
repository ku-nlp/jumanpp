//
// Created by Arseny Tolmachev on 2017/11/10.
//

#ifndef JUMANPP_NGRAM_FEATURE_CODEGEN_H
#define JUMANPP_NGRAM_FEATURE_CODEGEN_H

#include <string>
#include "core/spec/spec_types.h"
#include "util/printer.h"

namespace jumanpp {
namespace core {
namespace codegen {

namespace i = util::io;

class NgramFeatureCodegen {
  const spec::NgramFeatureDescriptor& ngram_;
  i32 target_;
  std::string name_;

 public:
  NgramFeatureCodegen(const spec::NgramFeatureDescriptor& ngram,
                      const spec::AnalysisSpec& spec);
  void makePartialObject(i::Printer& p) const;
  std::string rawUniStep0(i::Printer& p, StringPiece t0,
                          StringPiece mask) const;
  void biStep0(i::Printer& p, StringPiece patterns, StringPiece state) const;
  void triStep0(i::Printer& p, StringPiece patterns, StringPiece state) const;
  StringPiece name() const { return name_; }
};

}  // namespace codegen
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_NGRAM_FEATURE_CODEGEN_H
