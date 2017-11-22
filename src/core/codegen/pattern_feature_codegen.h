//
// Created by Arseny Tolmachev on 2017/11/09.
//

#ifndef JUMANPP_PATTERN_FEATURE_CODEGEN_H
#define JUMANPP_PATTERN_FEATURE_CODEGEN_H

#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "core/spec/spec_types.h"
#include "util/flatmap.h"
#include "util/printer.h"
#include "util/string_piece.h"

namespace jumanpp {
namespace core {
namespace codegen {

namespace i = util::io;

inline void concatImpl(std::stringstream& os) {}

template <typename Arg, typename... Args>
inline void concatImpl(std::stringstream& base, const Arg& a,
                       const Args&... args) {
  base << a;
  concatImpl(base, args...);
}

template <typename... Args>
inline std::string concat(const Args&... args) {
  std::stringstream s;
  concatImpl(s, args...);
  return s.str();
}

class InNodeComputationsCodegen {
  const spec::AnalysisSpec& spec_;
  util::FlatMap<i32, std::string> primitiveNames_;
  i32 numUnigrams_;
  i32 numBigrams_;
  i32 numTrigrams_;

  std::string patternValue(i::Printer& p,
                           const spec::PatternFeatureDescriptor& pat);
  void printCompute(i::Printer& p,
                    const spec::ComputationFeatureDescriptor& cfd,
                    StringPiece patternName);
  void ensureCompute(i::Printer& p,
                     const spec::ComputationFeatureDescriptor& cfd);
  void ensurePrimitive(i::Printer& p, i32 primitiveIdx);
  std::string primFeatureObjectName(
      i::Printer& p, const spec::PrimitiveFeatureDescriptor& pfd);
  StringPiece lengthTypeOf(i32 dicField) const;

 public:
  InNodeComputationsCodegen(const spec::AnalysisSpec& spec);
  void generateLoopBody(i::Printer& p);
  void generateLoop(i::Printer& p);
  void generate(i::Printer& p, StringPiece className);
};

}  // namespace codegen
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_PATTERN_FEATURE_CODEGEN_H
