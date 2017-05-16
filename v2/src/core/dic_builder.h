//
// Created by Arseny Tolmachev on 2017/02/21.
//

#ifndef JUMANPP_DIC_BUILDER_H
#define JUMANPP_DIC_BUILDER_H

#include <memory>
#include <vector>
#include "core/runtime_info.h"
#include "core/spec/spec_types.h"
#include "impl/model_format.h"
#include "util/status.hpp"

namespace jumanpp {
namespace core {
namespace dic {

struct BuiltField {
  StringPiece name;
  StringPiece stringContent;
  StringPiece fieldContent;
  spec::ColumnType colType;
  i32 uniqueValues;
  StringPiece emptyValue;
};

struct BuiltDictionary {
  StringPiece trieContent;
  StringPiece entryPointers;
  StringPiece entryData;
  std::vector<BuiltField> fieldData;
  i32 entryCount;
};

struct DictionaryBuilderStorage;

class DictionaryBuilder {
  spec::AnalysisSpec* spec_;
  std::unique_ptr<BuiltDictionary> dic_;
  std::unique_ptr<DictionaryBuilderStorage> storage_;

  Status fixupDictionary(const model::ModelPart& part);

 public:
  DictionaryBuilder();
  ~DictionaryBuilder();

  Status importSpec(spec::AnalysisSpec* spec);
  Status importCsv(StringPiece name, StringPiece data);
  const BuiltDictionary& result() const { return *dic_; }
  Status fillModelPart(const RuntimeInfo& info, model::ModelPart* part);
  Status restoreDictionary(const model::ModelInfo& info, RuntimeInfo* runtime);
  const spec::AnalysisSpec& spec() const { return *spec_; }
};

}  // namespace dic
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_DIC_BUILDER_H
