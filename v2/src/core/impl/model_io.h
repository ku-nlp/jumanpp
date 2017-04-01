//
// Created by Arseny Tolmachev on 2017/03/04.
//

#ifndef JUMANPP_MODEL_IO_H
#define JUMANPP_MODEL_IO_H

#include <memory>
#include "model_format.h"
#include "util/status.hpp"
#include "util/string_piece.h"

namespace jumanpp {
namespace core {
namespace model {

struct ModelFile;

class ModelSaver {
  std::unique_ptr<ModelFile> file_;

 public:
  ModelSaver(const ModelSaver&) = delete;
  ModelSaver();
  ~ModelSaver();
  Status open(StringPiece name);
  Status save(const ModelInfo& info);
};

class FilesystemModel {
  std::unique_ptr<ModelFile> file_;

 public:
  FilesystemModel(const FilesystemModel&) = delete;
  FilesystemModel();
  ~FilesystemModel();
  Status open(StringPiece name);
  Status load(ModelInfo* info);
};

}  // namespace model
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_MODEL_IO_H
