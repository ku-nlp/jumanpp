//
// Created by Arseny Tolmachev on 2017/07/20.
//

#ifndef JUMANPP_SUBSET_FORMAT_H
#define JUMANPP_SUBSET_FORMAT_H

#include "mdic_format.h"
#include "morph_format.h"

namespace jumanpp {
namespace jumandic {
namespace output {

class SubsetFormat : public core::OutputFormat {
  MorphFormat morph_{true};
  MdicFormat mdic_;
  std::string buffer_;

 public:
  Status initialize(const core::analysis::OutputManager& om) {
    JPP_RETURN_IF_ERROR(morph_.initialize(om));
    JPP_RETURN_IF_ERROR(mdic_.initialize(om));
    return Status::Ok();
  }

  Status format(const core::analysis::Analyzer& analyzer,
                StringPiece comment) override;

  StringPiece result() const override { return buffer_; }
};

}  // namespace output
}  // namespace jumandic
}  // namespace jumanpp

#endif  // JUMANPP_SUBSET_FORMAT_H
