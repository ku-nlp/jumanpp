//
// Created by Arseny Tolmachev on 2018/01/19.
//

#ifndef JUMANPP_PEX_STREAM_READER_H
#define JUMANPP_PEX_STREAM_READER_H

#include "stream_reader.h"

namespace jumanpp {
namespace core {
namespace input {

struct PexStreamReaderImpl;

/**
 * PEX = Partial Example
 */
class PexStreamReader : public StreamReader {
  std::unique_ptr<PexStreamReaderImpl> impl_;

 public:
  PexStreamReader();
  ~PexStreamReader() override;
  Status initialize(const CoreHolder &core, char32_t noBreak = U'&');
  Status initialize(const PexStreamReader& other);
  Status readExample(std::istream *stream) override;
  Status analyzeWith(analysis::Analyzer *an) override;
  StringPiece comment() override;
};

}  // namespace input
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_PEX_STREAM_READER_H
