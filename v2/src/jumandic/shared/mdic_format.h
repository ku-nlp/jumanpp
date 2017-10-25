//
// Created by Arseny Tolmachev on 2017/07/20.
//

#ifndef JUMANPP_MDIC_FORMAT_H
#define JUMANPP_MDIC_FORMAT_H

#include "juman_format.h"

namespace jumanpp {
namespace jumandic {
namespace output {

namespace fmt {
jumanpp::util::io::Printer& printMaybeQuoted(
    jumanpp::util::io::Printer& printer, StringPiece data);
jumanpp::util::io::Printer& printMaybeQuoteStringList(
    jumanpp::util::io::Printer& printer,
    core::analysis::StringListIterator items);
}  // namespace fmt

class MdicFormat : public core::OutputFormat {
  JumandicFields flds_;
  util::io::Printer printer_;
  std::array<i32, JumandicNumFields> fieldBuffer_;
  core::analysis::NodeWalker walker{&fieldBuffer_};
  util::FlatSet<i32> displayed_;

  Status formatNode(i32 nodeRawPtr, const core::analysis::OutputManager& om);

 public:
  Status initialize(const core::analysis::OutputManager& om);
  Status format(const core::analysis::Analyzer& analyzer,
                StringPiece comment) override;
  StringPiece result() const override;
};

}  // namespace output
}  // namespace jumandic
}  // namespace jumanpp

#endif  // JUMANPP_MDIC_FORMAT_H
