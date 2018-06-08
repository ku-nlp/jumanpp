//
// Created by Arseny Tolmachev on 2018/03/26.
//

#ifndef JUMANPP_GRPC_JUMAN_PB_FORMAT_H
#define JUMANPP_GRPC_JUMAN_PB_FORMAT_H

#include "juman_format.h"

namespace jumanpp {

// protobuf object
class JumanSentence;

namespace jumandic {

struct JumanPbFormatImpl;

class JumanPbFormat : public core::OutputFormat {
  std::unique_ptr<JumanPbFormatImpl> impl_;

 public:
  Status initialize(const core::analysis::OutputManager& om,
                    const JumandicIdResolver* resolver, bool fill);
  Status format(const core::analysis::Analyzer& analyzer,
                StringPiece comment) override;
  StringPiece result() const override;
  const JumanSentence* objectPtr() const;
  bool isInitialized() const { return impl_ != nullptr; }
  JumanPbFormat() noexcept;
  ~JumanPbFormat() override;
};

}  // namespace jumandic
}  // namespace jumanpp

#endif  // JUMANPP_GRPC_JUMAN_PB_FORMAT_H
