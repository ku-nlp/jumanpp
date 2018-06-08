//
// Created by Arseny Tolmachev on 2018/06/08.
//

#ifndef JUMANPP_INDEX_CMD_H
#define JUMANPP_INDEX_CMD_H

#include <memory>

#include "util/status.hpp"
#include "util/string_piece.h"

namespace jumanpp {
namespace core {

class ProgressCallback;

namespace tool {

struct IndexToolImpl;

class IndexTool {
  std::unique_ptr<IndexToolImpl> impl_;

 public:
  IndexTool();
  ~IndexTool();

  void setProgressCallback(ProgressCallback* callback);
  Status indexDictionary(StringPiece specFile, StringPiece dicFile);
  Status saveModel(StringPiece outputFile, StringPiece dicComment);
};

}  // namespace tool
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_INDEX_CMD_H
