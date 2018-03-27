//
// Created by Arseny Tolmachev on 2018/03/23.
//

#ifndef JUMANPP_GRPC_JUMANPP_PB_FORMAT_H
#define JUMANPP_GRPC_JUMANPP_PB_FORMAT_H

#include "core/env.h"
#include "jumandic/shared/jumandic_id_resolver.h"

namespace jumanpp {

// protobuf object
class Lattice;

namespace jumandic {

struct JumanppProtobufOutputImpl;

class JumanppProtobufOutput : public core::OutputFormat {
  std::unique_ptr<JumanppProtobufOutputImpl> impl_;

 public:
  Status initialize(const core::analysis::OutputManager& om,
                    const JumandicIdResolver* resolver, int topN, bool fill);
  Status format(const core::analysis::Analyzer& analyzer,
                StringPiece comment) override;
  StringPiece result() const override;
  const Lattice* objectPtr() const;
  bool isInitialized() const { return impl_ != nullptr; }
};

}  // namespace jumandic
}  // namespace jumanpp

#endif  // JUMANPP_GRPC_JUMANPP_PB_FORMAT_H
