//
// Created by Arseny Tolmachev on 2017/02/21.
//

#ifndef JUMANPP_DIC_BUILDER_H
#define JUMANPP_DIC_BUILDER_H

#include <memory>
#include <vector>
#include "core/impl/model_format.h"
#include "core/spec/spec_types.h"
#include "util/status.hpp"

namespace jumanpp {
namespace core {
namespace dic {

struct BuiltField {
  // SERIALIZED
  i32 dicIndex = spec::InvalidInt;
  i32 specIndex = spec::InvalidInt;
  i32 uniqueValues = spec::InvalidInt;

  // FILLED IN
  i32 stringStorageIdx;
  StringPiece stringContent;
  StringPiece fieldContent;
};

struct BuiltDictionary {
  StringPiece trieContent;
  StringPiece entryPointers;
  StringPiece entryData;
  std::vector<BuiltField> fieldData;
  std::vector<StringPiece> stringStorages;
  std::vector<StringPiece> intStorages;
  i32 entryCount = spec::InvalidInt;
  spec::AnalysisSpec spec;
  std::string comment;
  i64 timestamp = 0;
  Status restoreDictionary(const model::ModelInfo& info);
};

struct DictionaryBuilderStorage;

class DictionaryBuilder {
  spec::AnalysisSpec* spec_;
  std::unique_ptr<BuiltDictionary> dic_;
  std::unique_ptr<DictionaryBuilderStorage> storage_;

 public:
  DictionaryBuilder();
  ~DictionaryBuilder();

  Status fillModelPart(model::ModelPart* part);
  Status importSpec(spec::AnalysisSpec* spec);
  Status importCsv(StringPiece name, StringPiece data);
  const BuiltDictionary& result() const { return *dic_; }
  const spec::AnalysisSpec& spec() const { return *spec_; }
};

}  // namespace dic
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_DIC_BUILDER_H
