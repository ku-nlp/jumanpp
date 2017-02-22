//
// Created by Arseny Tolmachev on 2017/02/21.
//

#ifndef JUMANPP_DIC_BUILDER_H
#define JUMANPP_DIC_BUILDER_H

#include "dic_spec.h"
#include <vector>
#include <memory>

namespace jumanpp {
namespace core {
namespace dic {

struct BuiltField {
  StringPiece name;
  StringPiece stringContent;
  StringPiece fieldContent;
  bool usesFieldContent;

  i32 uniqueValues;
};

struct BuiltDictionary {
  StringPiece dicSpec;
  StringPiece trieContent;
  StringPiece entryPointers;
  StringPiece entryData;
  std::vector<BuiltField> fieldData;

  i32 entryCount;
};

struct DictionaryBuilderStorage;

class DictionaryBuilder {
  std::string specContent_;
  std::unique_ptr<DictionarySpec> spec_;
  std::unique_ptr<BuiltDictionary> dic_;
  std::unique_ptr<DictionaryBuilderStorage> storage_;

 public:
  DictionaryBuilder();
  ~DictionaryBuilder();

  Status importSpec(StringPiece name, StringPiece data);
  Status importCsv(StringPiece name, StringPiece data);
  const BuiltDictionary& result() const { return *dic_; }
};

}  // dic
}  // core
}  // jumanpp

#endif  // JUMANPP_DIC_BUILDER_H
